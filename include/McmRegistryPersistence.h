#pragma once

#include <windows.h>
#include <string>
#include <string_view>

namespace k2040
{
    struct McmRegistrySaveResult
    {
        bool saved = false;
        bool usedInPlaceFallback = false;
        bool restoreFailed = false;
        DWORD replaceError = ERROR_SUCCESS;
        DWORD fallbackError = ERROR_SUCCESS;
    };

    inline bool ReadMcmRegistryHandle(HANDLE handle, std::string& result)
    {
        LARGE_INTEGER fileSize{};
        if (!GetFileSizeEx(handle, &fileSize) || fileSize.QuadPart < 0 ||
            fileSize.QuadPart > 1024 * 1024) return false;
        if (!SetFilePointerEx(handle, LARGE_INTEGER{}, nullptr, FILE_BEGIN)) return false;
        result.resize(static_cast<std::size_t>(fileSize.QuadPart));
        DWORD read = 0;
        return result.empty() ||
            (ReadFile(handle, result.data(), static_cast<DWORD>(result.size()), &read, nullptr) &&
             read == result.size());
    }

    inline bool WriteMcmRegistryHandle(HANDLE handle, std::string_view content)
    {
        if (content.size() > 1024 * 1024 ||
            !SetFilePointerEx(handle, LARGE_INTEGER{}, nullptr, FILE_BEGIN)) return false;
        DWORD written = 0;
        if (!content.empty() &&
            (!WriteFile(handle, content.data(), static_cast<DWORD>(content.size()), &written, nullptr) ||
             written != content.size())) return false;
        return SetEndOfFile(handle) && FlushFileBuffers(handle);
    }

    // MCM may keep Keybinds.json open without FILE_SHARE_DELETE on Wine/Proton.
    // An atomic rename then fails even though writing the file is allowed.
    // Always attempt atomic replacement first. Only for access/sharing errors,
    // preserve a byte-verified recovery copy and modify the existing file.
    // If the current bytes changed since capture, fail closed without writing.
    inline McmRegistrySaveResult SaveSharedMcmRegistry(
        const std::wstring& temporary,
        const std::wstring& destination,
        std::string_view original,
        std::string_view replacement)
    {
        McmRegistrySaveResult result;
        if (MoveFileExW(temporary.c_str(), destination.c_str(),
            MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            result.saved = true;
            return result;
        }
        result.replaceError = GetLastError();
        if (result.replaceError != ERROR_ACCESS_DENIED &&
            result.replaceError != ERROR_SHARING_VIOLATION) return result;

        const auto recovery = destination + L".k2040-backup";
        // Backup is checked against the snapshot before anything is modified.
        if (!CopyFileW(destination.c_str(), recovery.c_str(), FALSE)) {
            result.fallbackError = GetLastError();
            return result;
        }
        const auto backup = CreateFileW(recovery.c_str(), GENERIC_READ,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (backup == INVALID_HANDLE_VALUE) {
            result.fallbackError = GetLastError();
            return result;
        }
        std::string backedUp;
        const bool verified = ReadMcmRegistryHandle(backup, backedUp) && backedUp == original;
        CloseHandle(backup);
        if (!verified) {
            result.fallbackError = ERROR_INVALID_DATA;
            return result;
        }

        const auto target = CreateFileW(destination.c_str(), GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (target == INVALID_HANDLE_VALUE) {
            result.fallbackError = GetLastError();
            return result;
        }
        std::string current;
        if (!ReadMcmRegistryHandle(target, current) || current != original) {
            result.fallbackError = ERROR_INVALID_DATA;
        } else {
            std::string confirmed;
            if (WriteMcmRegistryHandle(target, replacement) &&
                ReadMcmRegistryHandle(target, confirmed) && confirmed == replacement) {
                result.saved = true;
                result.usedInPlaceFallback = true;
            } else {
                result.fallbackError = GetLastError();
                if (result.fallbackError == ERROR_SUCCESS)
                    result.fallbackError = ERROR_WRITE_FAULT;
                result.restoreFailed = !WriteMcmRegistryHandle(target, original);
            }
        }
        CloseHandle(target);
        if (result.saved) DeleteFileW(temporary.c_str());
        return result;
    }
}

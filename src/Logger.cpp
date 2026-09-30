#include "Logger.h"

#include <filesystem>
#include <fstream>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string>

#ifndef K2040_QUICK_ATTACH_MENU_VERSION
#  define K2040_QUICK_ATTACH_MENU_VERSION "unknown"
#endif

namespace
{
    std::mutex g_logMutex;
    std::ofstream g_logFile;
    bool g_initialized = false;

    std::string SanitizeForFilename(std::string value)
    {
        for (char& c : value) {
            if (c == '.' || c == ' ' || c == '-' || c == '+') {
                c = '_';
            }
        }

        return value;
    }

    std::filesystem::path LogPath()
    {
        const std::string version = SanitizeForFilename(K2040_QUICK_ATTACH_MENU_VERSION);
        const std::string filename = std::string("K2040_Quick_Attach_Menu_v") + version + ".log";

        return std::filesystem::path("Data") / "F4SE" / "Plugins" / filename;
    }

    std::string Prefix(std::string_view level)
    {
        return "[" + std::string(level) + "] ";
    }

    void Write(std::string_view level, std::string_view message)
    {
        std::scoped_lock lock(g_logMutex);

        if (!g_initialized) {
            std::filesystem::create_directories(LogPath().parent_path());
            g_logFile.open(LogPath(), std::ios::out | std::ios::app);
            g_initialized = true;
        }

        if (g_logFile.is_open()) {
            g_logFile << Prefix(level) << message << '\n';
            g_logFile.flush();
        }

        if (level == "ERROR") {
            spdlog::error("{}", message);
        } else if (level == "WARN") {
            spdlog::warn("{}", message);
        } else {
            spdlog::info("{}", message);
        }
    }
}

namespace k2040::log
{
    void Init()
    {
        std::scoped_lock lock(g_logMutex);

        if (g_initialized) {
            return;
        }

        std::filesystem::create_directories(LogPath().parent_path());
        g_logFile.open(LogPath(), std::ios::out | std::ios::trunc);
        g_initialized = true;

        if (g_logFile.is_open()) {
            g_logFile << "[INFO] K2040's Quick Attach Menu log started." << '\n';
            g_logFile << "[INFO] Build version: " << K2040_QUICK_ATTACH_MENU_VERSION << '\n';
            g_logFile << "[INFO] Log file: " << LogPath().string() << '\n';
            g_logFile.flush();
        }

        spdlog::info("Quick Attach Menu diagnostic logging active for build {}.", K2040_QUICK_ATTACH_MENU_VERSION);
    }

    void Info(std::string_view message)
    {
        Write("INFO", message);
    }

    void Warn(std::string_view message)
    {
        Write("WARN", message);
    }

    void Error(std::string_view message)
    {
        Write("ERROR", message);
    }
}

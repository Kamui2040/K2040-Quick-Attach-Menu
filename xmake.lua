-- K2040_Quick_Attach_Menu / xmake.lua
-- Linux/Bazzite maintainer workflow; Windows x64 F4SE target.
--
-- Build from the repository root:
--   export K2040_COMMONLIBF4_ROOT="/path/to/commonlibf4"
--   xmake f -p windows -a x64 -m releasedbg
--   xmake -r
--
-- PrismaUI 2.1.1 compatibility review.
-- Builds stage into dist/Data only. Deployment is always explicit.

set_xmakever("3.0.0")
set_project("K2040_Quick_Attach_Menu")
set_version("0.5.181")
set_languages("c++23")

add_rules("mode.release", "mode.releasedbg", "mode.debug")

local commonlibf4 = os.getenv("K2040_COMMONLIBF4_ROOT")
if not commonlibf4 or commonlibf4 == "" then
    raise("K2040_COMMONLIBF4_ROOT must point to an xmake-compatible CommonLibF4 checkout")
end

commonlibf4 = path.absolute(commonlibf4)
if not os.isdir(commonlibf4) then
    raise("CommonLibF4 directory not found: " .. commonlibf4)
end
if not os.isfile(path.join(commonlibf4, "xmake.lua")) then
    raise("CommonLibF4 xmake.lua not found: " .. path.join(commonlibf4, "xmake.lua"))
end

local prisma_api_dir = path.join(os.scriptdir(), "external", "prismaui_f4")
local prisma_api_header = path.join(prisma_api_dir, "PrismaUI_F4_API.h")
local windows_sdk_include_dirs = {}

-- Native Linux clang-cl builds need the extracted Windows SDK include roots
-- passed to llvm-rc as well as to the C++ compiler. XMake discovers the C++
-- paths from --sdk, but does not currently forward them to the resource step.
if is_host("linux") and is_plat("windows") then
    local sdk_root = get_config("sdk")
    if sdk_root and sdk_root ~= "" then
        local version_roots = os.dirs(path.join(sdk_root, "Windows Kits", "10", "Include", "*"))
        table.sort(version_roots)
        local version_root = version_roots[#version_roots]
        if version_root then
            for _, component in ipairs({ "shared", "um", "ucrt" }) do
                local include_dir = path.join(version_root, component)
                if os.isdir(include_dir) then
                    table.insert(windows_sdk_include_dirs, include_dir)
                end
            end
        end
    end
    if #windows_sdk_include_dirs ~= 3 then
        raise("The configured Windows SDK does not expose shared, um, and ucrt include directories")
    end
end

if not os.isfile(prisma_api_header) then
    raise("PrismaUI API header not found: " .. prisma_api_header)
end

includes(commonlibf4)

-- XMake's Windows linker rule adds /opt:ref and /opt:icf for releasedbg.
-- Those switches belong on the final DLL, not on CommonLib's static archives.
-- Keeping archive stripping disabled avoids passing linker-only flags to the
-- Linux-native archive step while preserving releasedbg optimization here.
if is_mode("releasedbg") then
    target("commonlib-shared")
        set_strip("none")
    target("commonlibf4")
        set_strip("none")
end

target("K2040_Quick_Attach_Menu")
    set_kind("shared")
    set_languages("c++23")
    set_filename("K2040_Quick_Attach_Menu.dll")
    set_symbols("debug")

    add_rules("commonlibf4.plugin", {
        name    = "K2040_Quick_Attach_Menu",
        author  = "K2040",
        version = "0.5.181"
    })

    add_includedirs("include")
    add_includedirs("external/prismaui_f4")
    for _, include_dir in ipairs(windows_sdk_include_dirs) do
        add_includedirs(include_dir, { system = true })
    end
    add_files("src/**.cpp")

    add_defines(
        "WIN32_LEAN_AND_MEAN",
        "NOMINMAX",
        "COMMONLIB_RUNTIMECOUNT=3",
        "K2040_QUICK_ATTACH_MENU_VERSION=\"0.5.181\""
    )

    if is_plat("windows") then
        add_cxflags("/permissive-", "/wd4200", "/wd4201", "/wd4324")
        add_cxflags("/bigobj", "/FS")
        add_syslinks("version", "ole32", "oleaut32", "user32", "bcrypt", "crypt32")
    end

    after_build(function(target)
        local staged_plugins = path.join(os.scriptdir(), "dist", "Data", "F4SE", "Plugins")
        os.mkdir(staged_plugins)
        os.cp(target:targetfile(), staged_plugins)

        print("Built DLL staged to:")
        print("  " .. path.join(staged_plugins, "K2040_Quick_Attach_Menu.dll"))
    end)

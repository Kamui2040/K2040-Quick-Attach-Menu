#include "ControllerShortcuts.h"

#include "Logger.h"

#include <Windows.h>
#include <Xinput.h>

#include <array>
#include <charconv>
#include <mutex>
#include <sstream>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cmath>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <string>
#include <string_view>

namespace
{
    constexpr std::uint32_t kLeftTrigger = 0x10000u;
    constexpr std::uint32_t kRightTrigger = 0x20000u;
    // This legacy file is read once to migrate a pre-MCM user preference.
    // All subsequent edits use the child-owned ModSettingInt MCM user INI.
    constexpr auto kSettingsFile = "Data/F4SE/Plugins/K2040_Quick_Attach_Menu/ControllerShortcuts.ini";
    const auto kMcmUser=std::filesystem::path("Data/MCM/Settings/K2040_Quick_Attach_Menu.ini");
    const auto kMcmDefaults=std::filesystem::path("Data/MCM/Config/K2040_Quick_Attach_Menu/settings.ini");
    const auto kMinimapUser=std::filesystem::path("Data/MCM/Settings/K2040_Prisma_Minimap.ini");
    const auto kJournalUser=std::filesystem::path("Data/MCM/Settings/K2040_Quest_Journal.ini");
    const auto kMinimapConfig=std::filesystem::path("Data/MCM/Config/K2040_Prisma_Minimap/config.json");
    const auto kJournalConfig=std::filesystem::path("Data/MCM/Config/K2040_Quest_Journal/config.json");
    constexpr std::size_t kMaxMcmIni=128*1024;
    std::mutex g_bindingLock;
    std::atomic_bool g_conflict{false};
    std::chrono::steady_clock::time_point g_nextMcmRefresh{};
    constexpr std::array<std::string_view,17> kDropNames={
        "None","A","B","X","Y","DUp","DDown","DLeft","DRight",
        "LB","RB","LT","RT","Start","Back","LS","RS"
    };

    struct Button
    {
        std::string_view name;
        std::uint32_t mask;
    };

    constexpr std::array<Button, 16> kButtons{{
        { "A", XINPUT_GAMEPAD_A }, { "B", XINPUT_GAMEPAD_B },
        { "X", XINPUT_GAMEPAD_X }, { "Y", XINPUT_GAMEPAD_Y },
        { "LB", XINPUT_GAMEPAD_LEFT_SHOULDER }, { "RB", XINPUT_GAMEPAD_RIGHT_SHOULDER },
        { "LT", kLeftTrigger }, { "RT", kRightTrigger },
        { "Back", XINPUT_GAMEPAD_BACK }, { "Start", XINPUT_GAMEPAD_START },
        { "LS", XINPUT_GAMEPAD_LEFT_THUMB }, { "RS", XINPUT_GAMEPAD_RIGHT_THUMB },
        { "DUp", XINPUT_GAMEPAD_DPAD_UP }, { "DDown", XINPUT_GAMEPAD_DPAD_DOWN },
        { "DLeft", XINPUT_GAMEPAD_DPAD_LEFT }, { "DRight", XINPUT_GAMEPAD_DPAD_RIGHT }
    }};

    std::string g_quickBinding = "None";
    std::atomic<std::uint32_t> g_quickMask{ 0 };
    std::atomic_bool g_requireControllerRelease{ true };

    std::optional<std::uint32_t> ParseBinding(std::string_view value)
    {
        if (value == "None") return 0;
        if (value.empty() || value.size() > 24) return std::nullopt;
        const auto separator = value.find('+');
        if (separator != std::string_view::npos &&
            (separator == 0 || separator == value.size() - 1 ||
             value.find('+', separator + 1) != std::string_view::npos)) {
            return std::nullopt;
        }

        const auto first = separator == std::string_view::npos ? value : value.substr(0, separator);
        const auto second = separator == std::string_view::npos ? std::string_view{} : value.substr(separator + 1);
        const auto toMask = [](std::string_view text) {
            for (const auto& button : kButtons) {
                if (button.name == text) return button.mask;
            }
            return std::uint32_t{ 0 };
        };
        const auto a = toMask(first);
        if (!a) return std::nullopt;
        if (separator == std::string_view::npos) return a;
        const auto b = toMask(second);
        if (!b || b == a) return std::nullopt;
        return a | b;
    }

    std::optional<std::string> BoundedIni(const std::filesystem::path& path) {
        std::error_code ec;
        if(!std::filesystem::is_regular_file(path,ec)||ec||
           std::filesystem::file_size(path,ec)>kMaxMcmIni||ec) return std::nullopt;
        std::ifstream file(path,std::ios::binary);
        if(!file)return std::nullopt;
        std::string out((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
        if(out.size()>kMaxMcmIni || (!file.good()&&!file.eof()))return std::nullopt;
        return out;
    }
    std::string Trim(std::string t){
        const auto good=[](unsigned char c){return c!=' '&&c!='\r'&&c!='\n'&&c!='\t';};
        auto first=std::find_if(t.begin(),t.end(),good);
        auto last=std::find_if(t.rbegin(),t.rend(),good).base();
        return first<last?std::string(first,last):std::string{};
    }
    unsigned IniValue(const std::string& text,std::string_view key){
        std::istringstream input(text);
        std::string line;
        bool controller=false;
        while(std::getline(input,line)){
            const auto v=Trim(line);
            if(v.starts_with("[")&&v.ends_with("]")){controller=v=="[Controller]";continue;}
            if(!controller || v.starts_with(";") || v.starts_with("#"))continue;
            const auto pos=v.find('=');
            if(pos==v.npos || Trim(v.substr(0,pos))!=key)continue;
            const auto str=Trim(v.substr(pos+1));
            unsigned result{};
            const auto parsed=std::from_chars(str.data(),str.data()+str.size(),result);
            return parsed.ec==std::errc{}&&parsed.ptr==str.data()+str.size()&&result<17?result:0;
        }
        return 0;
    }
    std::optional<std::pair<unsigned,unsigned>> Indices(std::string_view binding){
        if(binding=="None")return std::pair{0u,0u};
        const auto plus=binding.find('+');
        const auto first=binding.substr(0,plus);
        const auto second=plus==binding.npos ? std::string_view{} : binding.substr(plus+1);
        auto find=[](std::string_view name)->unsigned{
            for(unsigned i=1;i<kDropNames.size();++i)if(kDropNames[i]==name)return i;
            return 0;
        };
        const auto held=plus==binding.npos?0:find(first);
        const auto primary=find(plus==binding.npos?first:second);
        if(!primary || (plus!=binding.npos&&!held) || primary==held)return std::nullopt;
        return std::pair{primary,held};
    }
    std::string BindingName(unsigned primary,unsigned held){
        if(primary>=kDropNames.size()||held>=kDropNames.size()||(!primary&&held)||primary==held)
            return "None";
        if(!primary)return "None";
        return held ? std::string(kDropNames[held])+"+"+std::string(kDropNames[primary])
                    : std::string(kDropNames[primary]);
    }
    std::optional<std::string> McmBinding(){
        const auto ini=BoundedIni(kMcmUser);
        if(!ini)return std::nullopt;
        return BindingName(IniValue(*ini,"iQuickAttachPadPrimary"),
                           IniValue(*ini,"iQuickAttachPadModifier"));
    }
    bool ConflictsWithOtherMods(unsigned primary,unsigned held=0){
        if(!primary)return false;
        // Fail closed for overlapping physical buttons, not just exact chords:
        // LB+RB cannot coexist safely with RB, RB+LB, or another LB+RB.
        const auto collides=[&](const std::string& ini,
                                std::string_view primaryName,std::string_view heldName){
            const auto otherPrimary=IniValue(ini,primaryName);
            const auto otherHeld=IniValue(ini,heldName);
            if(!otherPrimary)return false;
            return primary==otherPrimary || (held && held==otherPrimary) ||
                (otherHeld && (primary==otherHeld || (held&&held==otherHeld)));
        };
        std::error_code ec;
        if(std::filesystem::is_regular_file(kMinimapConfig,ec)&&!ec) {
            if(const auto ini=BoundedIni(kMinimapUser))
                if(collides(*ini,"iMinimapPadPrimary","iMinimapPadModifier"))return true;
        }
        if(std::filesystem::is_regular_file(kJournalConfig,ec)&&!ec) {
            if(const auto ini=BoundedIni(kJournalUser))
                if(collides(*ini,"iQuestJournalPadPrimary","iQuestJournalPadModifier")||
                   collides(*ini,"iQuestTrackerPadPrimary","iQuestTrackerPadModifier"))return true;
        }
        return false;
    }
    void Assign(std::string binding){
        auto mask=ParseBinding(binding).value_or(0);
        const auto indices=Indices(binding);
        g_conflict.store(indices && ConflictsWithOtherMods(indices->first,indices->second));
        if(g_conflict)mask=0;
        std::lock_guard guard(g_bindingLock);
        g_quickBinding=std::move(binding);
        g_quickMask.store(mask);
        g_requireControllerRelease.store(true);
    }
    void RefreshFromMcm(){
        const auto next=std::chrono::steady_clock::now();
        if(next<g_nextMcmRefresh)return;
        g_nextMcmRefresh=next+std::chrono::milliseconds(750);
        if(auto assigned=McmBinding()){
            bool changed=false;
            {std::lock_guard guard(g_bindingLock);changed=*assigned!=g_quickBinding;}
            const auto index=Indices(*assigned);
            if(changed || g_conflict.load() ||
               (index && ConflictsWithOtherMods(index->first,index->second)))
                Assign(*assigned);
        }
    }

    bool PersistBindings(std::string_view quick)
    {
        if(!std::filesystem::is_regular_file(kMcmDefaults))return false;
        const auto selection=Indices(quick);
        if(!selection)return false;
        if(ConflictsWithOtherMods(selection->first,selection->second))return false;
        const auto existing=BoundedIni(kMcmUser);
        if(!existing && std::filesystem::exists(kMcmUser))return false;

        std::string data;
        std::istringstream input(existing.value_or(""));
        std::string line;
        bool controller=false,found=false,donePrimary=false,doneHeld=false;
        const auto appendMissing=[&](){
            if(!donePrimary)data+="iQuickAttachPadPrimary="+std::to_string(selection->first)+"\r\n";
            if(!doneHeld)data+="iQuickAttachPadModifier="+std::to_string(selection->second)+"\r\n";
        };
        while(std::getline(input,line)){
            const auto t=Trim(line);
            if(t.starts_with("[")&&t.ends_with("]")){
                if(controller)appendMissing();
                controller=t=="[Controller]";if(controller)found=true;
            }
            const auto key=t.substr(0,t.find('='));
            if(controller&&key=="iQuickAttachPadPrimary"){
                if(!donePrimary)data+="iQuickAttachPadPrimary="+std::to_string(selection->first)+"\r\n";
                donePrimary=true;
            }else if(controller&&key=="iQuickAttachPadModifier"){
                if(!doneHeld)data+="iQuickAttachPadModifier="+std::to_string(selection->second)+"\r\n";
                doneHeld=true;
            }else data+=line+"\r\n";
        }
        if(controller)appendMissing();
        if(!found){data+="[Controller]\r\n";appendMissing();}
        if(data.size()>kMaxMcmIni)return false;
        std::error_code ec;
        std::filesystem::create_directories(kMcmUser.parent_path(),ec);
        if(ec)return false;
        auto temporary=kMcmUser;
        temporary+=".k2040.tmp";
        {std::ofstream output(temporary,std::ios::trunc|std::ios::binary);
         if(!output)return false;
         output<<data;output.flush();if(!output.good())return false;}
        if(!MoveFileExW(temporary.wstring().c_str(),kMcmUser.wstring().c_str(),
                      MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){
            std::filesystem::remove(temporary,ec);
            return false;
        }
        return true;
    }

    using XInputStateFn = DWORD (WINAPI*)(DWORD, XINPUT_STATE*);

    XInputStateFn ResolveXInputState()
    {
        static XInputStateFn getState = [] {
            for (const wchar_t* library : { L"xinput1_4.dll", L"xinput1_3.dll", L"xinput9_1_0.dll" }) {
                auto* module = LoadLibraryW(library);
                if (!module) continue;
                if (auto* proc = GetProcAddress(module, "XInputGetState")) {
                    return reinterpret_cast<XInputStateFn>(proc);
                }
                FreeLibrary(module);
            }
            return static_cast<XInputStateFn>(nullptr);
        }();
        return getState;
    }

    std::uint32_t ReadControllerButtons()
    {
        auto getState = ResolveXInputState();
        if (!getState) return 0;

        // Read one physical/Steam Input virtual controller, never combine
        // buttons from different players into a synthetic shortcut.
        for (DWORD player = 0; player < XUSER_MAX_COUNT; ++player) {
            XINPUT_STATE state{};
            if (getState(player, &state) != ERROR_SUCCESS) continue;
            std::uint32_t down = state.Gamepad.wButtons;
            if (state.Gamepad.bLeftTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) down |= kLeftTrigger;
            if (state.Gamepad.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD) down |= kRightTrigger;
            if (down) return down;
        }
        return 0;
    }
}

namespace k2040
{
    void InitializeControllerShortcuts()
    {
        std::string quick = "None";
        std::ifstream source(kSettingsFile);
        if (source) {
            std::string line;
            while (std::getline(source, line)) {
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (line.starts_with("Quick=")) quick = line.substr(6);
            }
        }

        if (!ParseBinding(quick)) {
            log::Warn("Invalid Quick Menu controller shortcut; left unassigned.");
            quick = "None";
        }
        // Do not delete the existing user file. Import only if the new
        // child-owned MCM preferences have not already been saved.
        if(const auto native=McmBinding())quick=*native;
        else if(quick!="None" && std::filesystem::is_regular_file(kMcmDefaults))
            PersistBindings(quick);
        // Older files may contain Builder=; it is deliberately ignored.
        Assign(quick);
    }

    std::string GetControllerShortcut(std::string_view action)
    {
        if (action == "quick") {
            std::lock_guard guard(g_bindingLock);
            return g_quickBinding;
        }
        return "None";
    }

    bool SetControllerShortcut(std::string_view action, std::string_view binding)
    {
        const auto mask = ParseBinding(binding);
        if (action != "quick" || !mask) return false;
        if (!PersistBindings(binding)) return false;
        Assign(std::string(binding));
        return true;
    }

    bool ResetControllerShortcuts()
    {
        // New child preferences explicitly become Unbound, so an old
        // legacy shortcut is never silently restored after resetting.
        if(!PersistBindings("None"))return false;
        Assign("None");
        return true;
    }

    int ReadControllerStickSector()
    {
        const auto getState = ResolveXInputState();
        if (!getState) return -1;
        for (DWORD player = 0; player < XUSER_MAX_COUNT; ++player) {
            XINPUT_STATE state{};
            if (getState(player, &state) != ERROR_SUCCESS) continue;
            const auto& pad = state.Gamepad;
            // Prefer the left stick, with right stick as a fallback.
            // A generous dead zone prevents accidental selection drift.
            const auto quantize = [](SHORT x, SHORT y) -> int {
                const auto xx = static_cast<double>(x);
                const auto yy = static_cast<double>(y);
                if (xx * xx + yy * yy < 12500.0 * 12500.0) return -1;
                constexpr double kPi = 3.14159265358979323846;
                double angle = std::atan2(xx, yy);
                if (angle < 0) angle += 2.0 * kPi;
                return static_cast<int>(angle * 72.0 / (2.0 * kPi)) % 72;
            };
            const int left = quantize(pad.sThumbLX, pad.sThumbLY);
            if (left >= 0) return left;
            return quantize(pad.sThumbRX, pad.sThumbRY);
        }
        return -1;
    }

    void PollControllerShortcutEdges(bool suppressed, bool& quick)
    {
        quick = false;
        RefreshFromMcm(); // MCM dropdown changes become effective without restart
        const auto down = ReadControllerButtons();
        static std::uint32_t previous = 0;
        const auto before = previous;
        previous = down;

        // A newly assigned shortcut never fires from buttons held during its
        // assignment. Both buttons must be released before activation.
        if (g_requireControllerRelease.load()) {
            if (down == 0) g_requireControllerRelease = false;
            return;
        }
        if (suppressed || down == 0) return;

        const auto quickMask = g_quickMask.load();
        quick = quickMask && (down & quickMask) == quickMask &&
            (before & quickMask) != quickMask;
    }
}

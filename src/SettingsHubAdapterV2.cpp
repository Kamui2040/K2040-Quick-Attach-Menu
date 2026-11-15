#include "K2040SettingsAdapterV2.h"
#include "JsonPreferencesParser.h"
#include "PrismaBridge.h"
#include "UserSettings.h"
#include "Logger.h"
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace {
    std::string Quote(std::string_view source) {
        std::string out="\"";
        for(unsigned char ch:source){
            if(ch=='"')out+="\\\"";
            else if(ch=='\\')out+="\\\\";
            else if(ch=='\n')out+="\\n";
            else if(ch<32){char part[7];std::snprintf(part,sizeof(part),"\\u%04x",ch);out+=part;}
            else out+=static_cast<char>(ch);
        }
        return out+"\"";
    }
    bool K2040_SETTINGS_V2_CALL Ready() noexcept {
        return k2040::GetPrismaBridge().IsAvailable();
    }
    const char* K2040_SETTINGS_V2_CALL Manifest() noexcept {
        static std::string text;
        if(!text.empty())return text.c_str();
        const auto filePath=std::filesystem::path("Data")/"PrismaUI_F4"/"views"/
            "K2040_Quick_Attach_Menu"/"settings"/"settings-v1-manifest.json";
        std::error_code ec;
        const auto length=std::filesystem::file_size(filePath,ec);
        if(ec||length==0||length>131072)return nullptr;
        std::ifstream stream(filePath,std::ios::binary);
        if(!stream)return nullptr;
        auto loaded=std::string(std::istreambuf_iterator<char>(stream),std::istreambuf_iterator<char>());
        if(loaded.size()!=length)return nullptr;
        text=std::move(loaded);return text.c_str();
    }
    k2040::QuickMenuLayoutSettings SelectedLayout(const k2040::QuickMenuPreferences& p){
        if(p.presentation=="radial")return p.radial;
        if(p.presentation=="hybrid")return p.hybrid;
        if(p.presentation=="horizontal")return p.horizontal;
        if(p.presentation=="orbit")return p.orbit;
        if(p.presentation=="clock")return p.clock;
        return p.cascade;
    }
    k2040::QuickMenuLayoutSettings& EditableLayout(k2040::QuickMenuPreferences& p){
        if(p.presentation=="radial")return p.radial;
        if(p.presentation=="hybrid")return p.hybrid;
        if(p.presentation=="horizontal")return p.horizontal;
        if(p.presentation=="orbit")return p.orbit;
        if(p.presentation=="clock")return p.clock;
        return p.cascade;
    }
    const char* K2040_SETTINGS_V2_CALL Values() noexcept {
        static std::string json;
        const auto p=k2040::GetQuickMenuPreferences();
        const auto b=k2040::GetBracketedTextPreferences();
        const auto layout=SelectedLayout(p);
        std::ostringstream out;
        out<<std::setprecision(17);
        auto str=[&](const char* id,const std::string& value){out<<Quote(id)<<":"<<Quote(value)<<",";};
        auto boolean=[&](const char* id,bool value){out<<Quote(id)<<":"<<(value?"true":"false")<<",";};
        auto number=[&](const char* id,double value){out<<Quote(id)<<":"<<value<<",";};
        out<<"{";
        str("presentation",p.presentation);
        str("theme",p.theme);
        str("customAccent",p.customAccent);
        str("customText",p.customText);
        str("customPanel",p.customPanel);
        str("customInstalled",p.customInstalled);
        str("controlHints",p.controlHints);
        number("backgroundOpacity",p.backgroundOpacity);
        number("scale",layout.scale);
        number("positionX",layout.positionX);
        number("positionY",layout.positionY);
        number("menuSlowdown",p.menuSlowdown);
        boolean("hideBracketedPrefixes",b.hidePrefixes);
        boolean("hideBracketedInfixes",b.hideInfixes);
        boolean("hideBracketedSuffixes",b.hideSuffixes);
        boolean("shortenClockLabels",p.shortenClockLabels);
        boolean("useAuthoredMenus",p.useAuthoredMenus);
        boolean("closeAfterApply",p.closeAfterApply);
        boolean("allowMovementWhileOpen",p.allowMovementWhileOpen);
        boolean("cheatMode",p.cheatMode);
        boolean("loggingEnabled",p.loggingEnabled);
        json=out.str();if(!json.empty()&&json.back()==',')json.pop_back();json+="}";
        return json.c_str();
    }
    bool K2040_SETTINGS_V2_CALL Apply(const char* id,const char* rawValue) noexcept {
        if(!id||!rawValue)return false;
        const std::string_view name(id),input(rawValue);
        if(name.empty()||name.size()>80||input.size()>256)return false;
        const auto parsed=k2040::json::JsonParser(std::string(input)).Parse();
        if(!parsed)return false;
        const auto* text=parsed->AsString();
        const auto* number=parsed->AsNumber();
        const auto* flag=parsed->AsBool();
        if(!k2040::GetUserSettingsDiagnostics().saveAllowed)return false;
        auto p=k2040::GetQuickMenuPreferences();
        const auto before=p;
        if(name=="hideBracketedPrefixes"||name=="hideBracketedInfixes"||
           name=="hideBracketedSuffixes"){
            if(!flag)return false;
            auto b=k2040::GetBracketedTextPreferences();
            const auto prior=b;
            if(name=="hideBracketedPrefixes")b.hidePrefixes=*flag;
            if(name=="hideBracketedInfixes")b.hideInfixes=*flag;
            if(name=="hideBracketedSuffixes")b.hideSuffixes=*flag;
            if(b.hidePrefixes==prior.hidePrefixes&&b.hideInfixes==prior.hideInfixes&&
               b.hideSuffixes==prior.hideSuffixes)return true;
            if(!k2040::SetBracketedTextPreferences(b))return false;
            if(k2040::SaveUserPreferences())return true;
            (void)k2040::SetBracketedTextPreferences(prior);
            return false;
        }
        if(name=="presentation"&&text)p.presentation=*text;
        else if(name=="theme"&&text)p.theme=*text;
        else if(name=="customAccent"&&text)p.customAccent=*text;
        else if(name=="customText"&&text)p.customText=*text;
        else if(name=="customPanel"&&text)p.customPanel=*text;
        else if(name=="customInstalled"&&text)p.customInstalled=*text;
        else if(name=="controlHints"&&text)p.controlHints=*text;
        else if(name=="backgroundOpacity"&&number)p.backgroundOpacity=*number;
        else if(name=="menuSlowdown"&&number)p.menuSlowdown=*number;
        else if(name=="scale"&&number)EditableLayout(p).scale=*number;
        else if(name=="positionX"&&number)EditableLayout(p).positionX=*number;
        else if(name=="positionY"&&number)EditableLayout(p).positionY=*number;
        else if(name=="useAuthoredMenus"&&flag)p.useAuthoredMenus=*flag;
        else if(name=="shortenClockLabels"&&flag)p.shortenClockLabels=*flag;
        else if(name=="closeAfterApply"&&flag)p.closeAfterApply=*flag;
        else if(name=="allowMovementWhileOpen"&&flag)p.allowMovementWhileOpen=*flag;
        else if(name=="cheatMode"&&flag)p.cheatMode=*flag;
        else if(name=="loggingEnabled"&&flag)p.loggingEnabled=*flag;
        else return false;
        // Strict client-side and hub validation are also performed by the
        // shared renderer. Here, reject attempts to coerce or clamp values.
        if((name=="backgroundOpacity"&&(p.backgroundOpacity<0.25||p.backgroundOpacity>1.0))||
           (name=="menuSlowdown"&&(p.menuSlowdown<0.0||p.menuSlowdown>1.0))||
           (number&&!std::isfinite(*number)))return false;
        if(!k2040::SetQuickMenuPreferences(p)){
            // Distinguish an unchanged value from the owner's failed save gate.
            if(!k2040::GetUserSettingsDiagnostics().saveAllowed)return false;
            return true;
        }
        if(!k2040::SaveUserPreferences()){
            (void)k2040::SetQuickMenuPreferences(before);return false;
        }
        if(name=="loggingEnabled")k2040::log::SetEnabled(p.loggingEnabled);
        // Menu slowdown takes effect on the next menu open, the same path as
        // the normal settings page. Hub and Quick Menu never hold focus together.
        return true;
    }
    bool K2040_SETTINGS_V2_CALL Theme(const char* id) noexcept {
        return id&&k2040::GetPrismaBridge().ApplyUnifiedTheme(id);
    }
    bool K2040_SETTINGS_V2_CALL Reset(const char*) noexcept {
        // Existing reset-all operation also destroys weapon-specific profile
        // choices and requires an owning-module confirmation flow. Until that
        // confirmation API exists, the shared hub refuses it.
        return false;
    }
}
extern "C" __declspec(dllexport)
const K2040SettingsAdapterV2* K2040_SETTINGS_V2_CALL
K2040_GetSettingsAdapterV2() noexcept {
    static const K2040SettingsAdapterV2 adapter{
        sizeof(K2040SettingsAdapterV2),K2040_SETTINGS_ADAPTER_V2_ABI,
        "quick-attach",Ready,Manifest,Values,Apply,Reset,Theme};
    return &adapter;
}

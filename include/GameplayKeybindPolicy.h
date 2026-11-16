#pragma once
// Independent Quick Attach gameplay chords; never mutate the shared MCM registry.
#include <algorithm>
#include <charconv>
#include <string>
#include <string_view>
#include <utility>

namespace k2040::gameplay_keys {
inline bool Valid(std::string_view chord) {
    if (chord == "Unbound") return true;
    if (chord.empty() || chord.size() > 32) return false;
    bool ctrl=false,shift=false,alt=false,key=false;
    while (!chord.empty()) {
        const auto end=chord.find('+');
        const auto part=chord.substr(0,end);
        if (part == "Ctrl") { if(ctrl||key)return false;ctrl=true; }
        else if (part == "Shift") {if(shift||key)return false;shift=true;}
        else if (part == "Alt") {if(alt||key)return false;alt=true;}
        else {
            if(key || end!=std::string_view::npos)return false;
            key=(part.size()==1 && ((part[0]>='A'&&part[0]<='Z')||
                (part[0]>='0'&&part[0]<='9'))) ||
                part=="Mouse3" || part=="Mouse4" || part=="Mouse5";
            if(part.size()>1 && part.front()=='F') {
                unsigned number=0;
                const auto digits=part.substr(1);
                const auto result=std::from_chars(digits.data(),digits.data()+digits.size(),number);
                if(result.ec==std::errc{} && result.ptr==digits.data()+digits.size() &&
                    number>=1 && number<=12)key=true;
            }
            if (!key)return false;
        }
        if(end==std::string_view::npos)break;
        chord.remove_prefix(end+1);
        if(chord.empty())return false;
    }
    // Do not shadow the hub's provisional Ctrl+Shift+B settings shortcut.
    return key && !(ctrl&&shift&&!alt&&chord=="B");
}
inline std::pair<unsigned,unsigned> ToKeycode(std::string_view text) {
    if (!Valid(text) || text=="Unbound") return {0,0};
    unsigned modifiers=0,code=0;
    while (!text.empty()) {
        const auto end=text.find('+');
        const auto part=text.substr(0,end);
        if(part=="Ctrl")modifiers|=2;
        else if(part=="Shift")modifiers|=1;
        else if(part=="Alt")modifiers|=4;
        else if(part.size()==1)code=static_cast<unsigned>(part.front());
        else if(part=="Mouse3")code=258;
        else if(part=="Mouse4")code=259;
        else if(part=="Mouse5")code=260;
        else if(part.size()>1 && part[0]=='F') {
            unsigned function=0;
            const auto digits=part.substr(1);
            const auto result=std::from_chars(digits.data(),digits.data()+digits.size(),function);
            if(result.ec!=std::errc{}||result.ptr!=digits.data()+digits.size())
                return {0,0};
            code=111+function;
        }
        if(end==std::string_view::npos)break;
        text.remove_prefix(end+1);
    }
    return {code,modifiers};
}
inline std::string FromKeycode(unsigned keycode,unsigned modifiers) {
    if(modifiers&~7u)return {};
    std::string key;
    if((keycode>=65&&keycode<=90)||(keycode>=48&&keycode<=57))key.push_back(char(keycode));
    else if(keycode>=112&&keycode<=123)key="F"+std::to_string(keycode-111);
    else if(keycode==258)key="Mouse3";
    else if(keycode==259)key="Mouse4";
    else if(keycode==260)key="Mouse5";
    else return {};
    const std::string chord=(modifiers&2u?"Ctrl+":"")+
        std::string(modifiers&1u?"Shift+":"")+
        std::string(modifiers&4u?"Alt+":"")+key;
    return Valid(chord)?chord:std::string{};
}
}

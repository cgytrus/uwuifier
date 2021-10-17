#include "includes.h"
#include <random>
#include <algorithm>
#include <fstream>
#include "hackpro_ext.h"

bool enabled = true;
bool enabledInLevels = false;
// multiplied by 10000
unsigned int periodToExclamationChance = 2000;
int stutterChance = 1000;
int presuffixChance = 1000;
int suffixChance = 2000;

static std::unordered_map<char, char> const SINGLE_REPLACE = {
    { 'l', 'w' },
    { 'r', 'w' }
};

static std::unordered_map<std::string, std::string> const REPLACE = {
    { " you ", " uwu " },
    { " no ", " nu " },
    { " na", " nya" },
    { " ne", " nye" },
    { " ni", " nyi" },
    { " no", " nyo" },
    { " nu", " nyu" },
    { "te", "twe" },
    { "da", "dwa" },
    { "ke", "kwe" },
    { "qe", "qwe" },
    { "je", "jwe" },
    { "ne", "nwe" },
    { "na", "nwa" },
    { "si", "swi" },
    { "so", "swo" },
    { "mi", "mwi" },
    { "co", "cwo" },
    { "mo", "mwo" },
    { "ba", "bwa" },
    { "pow", "paw" }
};

static std::vector<std::string> const PRESUFFIXES = {
    "~"
};

static std::vector<std::string> const SUFFIXES = {
    " :D",
    " xD",
    " :P",
    " ;3",
    " <{^v^}>",
    " ^-^",
    " x3",
    " x3",
    " rawr",
    " rawr x3",
    " owo",
    " uwu",
    " -.-",
    " >w<",
    " :3",
    " XD",
    " nyaa~~",
    " >_<",
    " :flushed:",
    " ^^",
    " ^^;;"
};

void replaceAll(std::string& inout, std::string_view what, std::string_view with) {
    std::size_t count{};
    for(std::string::size_type pos{}; inout.npos != (pos = inout.find(what.data(), pos, what.length())); pos += with.length(), ++count) {
        inout.replace(pos, what.length(), with.data(), with.length());
    }
}

std::random_device::result_type rd = std::random_device()();
template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
    std::hash<T> hasher;
    seed ^= hasher(v) + rd + (seed << 6) + (seed >> 2);
}

inline bool getChance(int chance) { return std::rand() % 10000 < chance; }
inline const char* chanceToCString(int chance) {
    char array[7] = { '\0', '\0', '\0', '\0', '\0', '\0', '\0' };
    sprintf_s(array, "%.2f", chance / 100.0);
    return (const char*)&array[0];
}
inline int cstringToChance(const char* cstring) {
    return (int)(atof(cstring) * 100.0);
}

std::string newString = std::string();
const char* uwuify(const char* originalString) {
    size_t origLength = strlen(originalString);
    bool isEmpty = true;
    for(size_t i = 0; i < origLength; i++) {
        if(!isspace(originalString[i])) isEmpty = false;
    }
    if(isEmpty) return originalString;

    newString.clear();

    // copy and replace . with !
    newString.append(" ");
    for(size_t i = 0; i < origLength; i++) {
        char prevChar = i == 0 ? ' ' : tolower(originalString[i - 1]);
        char currChar = tolower(originalString[i]);
        char nextChar = i == origLength - 1 ? ' ' : tolower(originalString[i + 1]);

        if(currChar == '.' && nextChar == ' ' && getChance(periodToExclamationChance)) currChar = '!';
        newString += currChar;
    }
    newString.append(" ");

    // replacements
    for(size_t i = 1; i < newString.size() - 1; i++) {
        char prevChar = newString[i - 1];
        char currChar = newString[i];
        char nextChar = newString[i + 1];

        if(!isalpha(prevChar) && !isalpha(nextChar) || !SINGLE_REPLACE.contains(currChar)) continue;
        newString[i] = SINGLE_REPLACE.at(currChar);
    }
    for(auto elem : REPLACE) {
        replaceAll(newString, elem.first, elem.second);
    }

    // stutter and suffixes
    for(size_t i = 1; i < newString.size(); i++) {
        bool end = i == newString.size() - 1;

        char prevChar = newString[i - 1];
        char currChar = newString[i];
        char nextChar = end ? ' ' : newString[i + 1];

        bool stutter = prevChar == ' ' && isalpha(currChar) && getChance(stutterChance);
        if(stutter) {
            newString.insert(i, 1, '-');
            newString.insert(i, 1, currChar);
        }

        int suffixLength = 0;
        if(currChar == ' ' && (prevChar == '.' || prevChar == '!' || prevChar == ',' || nextChar == '-' || end)) {
            if(getChance(suffixChance) && (!end || isalnum(prevChar))) {
                auto suffix = SUFFIXES[std::rand() % SUFFIXES.size()];
                newString.insert(i, suffix);
                suffixLength += suffix.size();
            }
            if(getChance(presuffixChance)) {
                auto presuffix = PRESUFFIXES[std::rand() % PRESUFFIXES.size()];
                newString.insert(prevChar != ',' || end ? i : i - 1, presuffix);
                suffixLength += presuffix.size();
            }
        }

        if(stutter) i += 2;
        i += suffixLength;
    }

    newString = newString.substr(1, newString.size() - 2);
    return newString.c_str();
}

bool ignoreUwuifying = false;
void (__thiscall* CCLabelBMFont_setString)(CCLabelBMFont* self, const char *newString, bool needUpdateLabel);
void __fastcall CCLabelBMFont_setString_H(CCLabelBMFont* self, void*, const char *newString, bool needUpdateLabel) {
    auto parent = self->getParent();
    // same as comment on the hook below
    if(!enabled || !enabledInLevels && (ignoreUwuifying || parent != nullptr && /*parent->getObjType() == 13*/ typeid(*parent) == typeid(gd::GameObject))) {
        CCLabelBMFont_setString(self, newString, needUpdateLabel);
        return;
    }

    auto seed = (unsigned int)rd;
    hash_combine(seed, self);
    std::srand(seed);
    CCLabelBMFont_setString(self, uwuify(newString), needUpdateLabel);
}

// don't uwuify the levels themselves, or they may break xd
void (__thiscall* GameObject_updateTextObject)(gd::GameObject* self, char param_1, std::string param_2);
void __fastcall GameObject_updateTextObject_H(gd::GameObject* self, void*, char param_1, std::string param_2) {
    ignoreUwuifying = true;
    GameObject_updateTextObject(self, param_1, param_2);
    ignoreUwuifying = false;
}

gd::GameObject* (__thiscall* LevelEditorLayer_addObjectFromString)(gd::LevelEditorLayer* self, std::string str);
gd::GameObject* __fastcall LevelEditorLayer_addObjectFromString_H(gd::LevelEditorLayer* self, void*, std::string str) {
    ignoreUwuifying = true;
    auto ret = LevelEditorLayer_addObjectFromString(self, str);
    ignoreUwuifying = false;
    return ret;
}

gd::GameObject* (__thiscall* LevelEditorLayer_createObject)(gd::LevelEditorLayer* self, int id, cocos2d::CCPoint position, bool undo);
gd::GameObject* __fastcall LevelEditorLayer_createObject_H(gd::LevelEditorLayer* self, void*, int id, cocos2d::CCPoint position, bool undo) {
    ignoreUwuifying = true;
    auto ret = LevelEditorLayer_createObject(self, id, position, undo);
    ignoreUwuifying = false;
    return ret;
}

void __stdcall extEnabledCallback(void* cb) { enabled = true; }
void __stdcall extDisabledCallback(void* cb) { enabled = false; }

void __stdcall extEnabledInLevelsCallback(void* cb) { enabledInLevels = true; }
void __stdcall extDisabledInLevelsCallback(void* cb) { enabledInLevels = false; }

// percents
void __stdcall extChangedPeriodToExclamationChance(void* tb) { periodToExclamationChance = cstringToChance(HackproGetTextBoxText(tb)); }
void __stdcall extChangedStutterChance(void* tb) { stutterChance = cstringToChance(HackproGetTextBoxText(tb)); }
void __stdcall extChangedPresuffixChance(void* tb) { presuffixChance = cstringToChance(HackproGetTextBoxText(tb)); }
void __stdcall extChangedSuffixChance(void* tb) { suffixChance = cstringToChance(HackproGetTextBoxText(tb)); }

DWORD WINAPI mainThread(void* hModule) {
    MH_Initialize();

    auto base = reinterpret_cast<uintptr_t>(GetModuleHandle(0));
    auto cocos2dBase = reinterpret_cast<uintptr_t>(GetModuleHandle("libcocos2d.dll"));

    MH_CreateHook(
        reinterpret_cast<void*>(cocos2dBase + 0x9fb60),
        CCLabelBMFont_setString_H,
        reinterpret_cast<void**>(&CCLabelBMFont_setString)
    );

    MH_CreateHook(
        reinterpret_cast<void*>(base + 0xcfc60),
        GameObject_updateTextObject_H,
        reinterpret_cast<void**>(&GameObject_updateTextObject)
    );

    MH_CreateHook(
        reinterpret_cast<void*>(base + 0x160c80),
        LevelEditorLayer_addObjectFromString_H,
        reinterpret_cast<void**>(&LevelEditorLayer_addObjectFromString)
    );

    MH_CreateHook(
        reinterpret_cast<void*>(base + 0x160d70),
        LevelEditorLayer_createObject_H,
        reinterpret_cast<void**>(&LevelEditorLayer_createObject)
    );

    MH_EnableHook(MH_ALL_HOOKS);

    if(InitialiseHackpro() && HackproIsReady()) {
        void* ext = HackproInitialiseExt(uwuify("uwuifier"));

        auto suffixChanceTb = HackproAddTextBox(ext, extChangedSuffixChance);
        HackproSetTextBoxPlaceholder(suffixChanceTb, uwuify("Suffix Chance"));
        HackproSetTextBoxText(suffixChanceTb, chanceToCString(suffixChance));

        auto presuffixChanceTb = HackproAddTextBox(ext, extChangedPresuffixChance);
        HackproSetTextBoxPlaceholder(presuffixChanceTb, uwuify("Presuffix Chance"));
        HackproSetTextBoxText(presuffixChanceTb, chanceToCString(presuffixChance));

        auto stutterChanceTb = HackproAddTextBox(ext, extChangedStutterChance);
        HackproSetTextBoxPlaceholder(stutterChanceTb, uwuify("Stutter Chance"));
        HackproSetTextBoxText(stutterChanceTb, chanceToCString(stutterChance));

        auto periodToExclamationChanceTb = HackproAddTextBox(ext, extChangedPeriodToExclamationChance);
        HackproSetTextBoxPlaceholder(periodToExclamationChanceTb, uwuify("Period To Exclamation Chance"));
        HackproSetTextBoxText(periodToExclamationChanceTb, chanceToCString(periodToExclamationChance));

        // for some reason if i uwuify this, it doesn't work
        auto enabledInLevelsCb = HackproAddCheckbox(ext, "Enabled In Levels", extEnabledInLevelsCallback, extDisabledInLevelsCallback);
        HackproSetCheckbox(enabledInLevelsCb, enabledInLevels);

        auto enabledCb = HackproAddCheckbox(ext, uwuify("Enabled"), extEnabledCallback, extDisabledCallback);
        HackproSetCheckbox(enabledCb, enabled);

        HackproCommitExt(ext);
    }

    return 0;
}

BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        CreateThread(0, 0x100, mainThread, handle, 0, 0);
    }
    return TRUE;
}
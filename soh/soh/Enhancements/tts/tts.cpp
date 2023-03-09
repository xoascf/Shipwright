#include "soh/Enhancements/game-interactor/GameInteractor.h"

#include <cassert>
#include <File.h>
#include <libultraship/bridge.h>
#include <libultraship/classes.h>
#include <nlohmann/json.hpp>

#include "soh/OTRGlobals.h"
#include "message_data_static.h"
#include "overlays/gamestates/ovl_file_choose/file_choose.h"
#include "soh/Enhancements/boss-rush/BossRush.h"
#include "soh/resource/type/SohResourceType.h"
#include "soh/resource/type/RawJson.h"

extern "C" {
extern MapData* gMapData;
extern SaveContext gSaveContext;
extern PlayState* gPlayState;
}

typedef enum {
    /* 0x00 */ TEXT_BANK_SCENES,
    /* 0x01 */ TEXT_BANK_MISC,
    /* 0x02 */ TEXT_BANK_KALEIDO,
    /* 0x03 */ TEXT_BANK_FILECHOOSE,
} TextBank;

const char* GetLanguageCode() {
    switch (CVarGetInteger("gLanguages", 0)) {
        case LANGUAGE_FRA:
            return "fr-FR";
            break;
        case LANGUAGE_GER:
            return "de-DE";
            break;
    }

    //return "en-US";
    return "es-ES";
}

std::string GetJSONLanguage() {
    //std::string lang = "eng";
    std::string lang = "spa";
    switch (CVarGetInteger("gLanguages", 0)) {
        case LANGUAGE_FRA:
            lang = "fra";
            break;
        case LANGUAGE_GER:
            lang = "ger";
            break;
    }

    return lang;
}

nlohmann::json sceneMap = nullptr;
nlohmann::json miscMap = nullptr;
nlohmann::json kaleidoMap = nullptr;
nlohmann::json fileChooseMap = nullptr;
nlohmann::json charSetMap = nullptr;

std::string langLoaded;
bool UpdateJSON(const std::string jsonFilePath, nlohmann::json* jsonMap, const std::string language) {
    auto initData = std::make_shared<LUS::ResourceInitData>();
    initData->Format = RESOURCE_FORMAT_BINARY;
    initData->Type = static_cast<uint32_t>(SOH::ResourceType::SOH_RawJson);
    initData->ResourceVersion = 0;

    auto map = std::static_pointer_cast<SOH::RawJson>(
        LUS::Context::GetInstance()->GetResourceManager()->LoadResource(jsonFilePath + "_" + language + ".json", true, initData))->Data;

    //nlohmann::json map = nlohmann::json::parse(*file->Buffer.get()); // Old way (worked before someone added "comments" in JSON)
    //nlohmann::json map = nlohmann::json::parse(*file->Buffer.get(), nullptr, true, true); //FIXME: Please don't use "//" comments in JSON!!
    jsonMap->update(map);
    return true;
}

void ParseJSON(const std::string jsonFilePath, nlohmann::json* jsonMap, const std::string lang) {
    if (UpdateJSON(jsonFilePath, jsonMap, "eng") && lang != "eng") { // Always load eng as fallback, if not present, don't load!
        UpdateJSON(jsonFilePath, jsonMap, lang);
    }
}

std::vector<std::pair<std::string, nlohmann::json*>> jsonFiles = {
    {"accessibility/texts/scenes", &sceneMap},
    {"accessibility/texts/misc", &miscMap},
    {"accessibility/texts/kaleidoscope", &kaleidoMap},
    {"accessibility/texts/filechoose", &fileChooseMap},
    {"accessibility/texts/charset", &charSetMap}
};

std::map<std::uint8_t, std::string> charReplacements;

void LoadAccessibilityTexts(std::string lang) {
    if (langLoaded == lang)
        return;

    langLoaded = lang;
    charReplacements.clear();
    for (auto& [jsonFilePath, jsonMap] : jsonFiles) {
        jsonMap->clear();
        ParseJSON(jsonFilePath, jsonMap, lang);
    }
    if (charSetMap.is_object() && !charSetMap.empty()) {
        for (auto& c : charSetMap.items()) {
            charReplacements[static_cast<uint8_t>(std::strtoul(c.key().c_str(), nullptr, 16))] = c.value();
        }
    }
}

// MARK: - Helpers

std::string GetParameterizedText(std::string key, TextBank bank, const char* arg) {
    switch (bank) {
        case TEXT_BANK_SCENES: {
            return sceneMap[key].get<std::string>();
            break;
        }
        case TEXT_BANK_MISC: {
            auto value = miscMap[key].get<std::string>();
            
            std::string searchString = "$0";
            size_t index = value.find(searchString);
            
            if (index != std::string::npos) {
                assert(arg != nullptr);
                value.replace(index, searchString.size(), std::string(arg));
                return value;
            } else {
                return value;
            }
            
            break;
        }
        case TEXT_BANK_KALEIDO: {
            auto value = kaleidoMap[key].get<std::string>();
            
            std::string searchString = "$0";
            size_t index = value.find(searchString);
            
            if (index != std::string::npos) {
                assert(arg != nullptr);
                value.replace(index, searchString.size(), std::string(arg));
                return value;
            } else {
                return value;
            }
            
            break;
        }
        case TEXT_BANK_FILECHOOSE: {
            auto value = fileChooseMap[key].get<std::string>();

            std::string searchString = "$0";
            size_t index = value.find(searchString);

            if (index != std::string::npos) {
                assert(arg != nullptr);
                value.replace(index, searchString.size(), std::string(arg));
                return value;
            } else {
                return value;
            }

            break;
        }
    }
}

// MARK: - Boss Title Cards

std::string NameForSceneId(int16_t sceneId) {
    auto key = std::to_string(sceneId);
    auto name = GetParameterizedText(key, TEXT_BANK_SCENES, nullptr);
    return name;
}

static std::string titleCardText;

void RegisterOnSceneInitHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSceneInit>([](int16_t sceneNum) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        titleCardText = NameForSceneId(sceneNum);
    });
}

void RegisterOnPresentTitleCardHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPresentTitleCard>([]() {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        SpeechSynthesizerSpeak(titleCardText, GetLanguageCode());
    });
}

// MARK: - Interface Updates

void RegisterOnInterfaceUpdateHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnInterfaceUpdate>([]() {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        static uint32_t prevTimer = 0;
        static char ttsAnnounceBuf[32];
        
        uint32_t timer = 0;
        if (gSaveContext.timer1State != 0) {
            timer = gSaveContext.timer1Value;
        } else if (gSaveContext.timer2State != 0) {
            timer = gSaveContext.timer2Value;
        }
        
        if (timer > 0) {
            if (timer > prevTimer || (timer % 30 == 0 && prevTimer != timer)) {
                uint32_t minutes = timer / 60;
                uint32_t seconds = timer % 60;
                char* announceBuf = ttsAnnounceBuf;
                char arg[8]; // at least big enough where no s8 string will overflow
                if (minutes > 0) {
                    snprintf(arg, sizeof(arg), "%d", minutes);
                    auto translation = GetParameterizedText((minutes > 1) ? "minutes_plural" : "minutes_singular", TEXT_BANK_MISC, arg);
                    announceBuf += snprintf(announceBuf, sizeof(ttsAnnounceBuf), "%s ", translation.c_str());
                }
                if (seconds > 0) {
                    snprintf(arg, sizeof(arg), "%d", seconds);
                    auto translation = GetParameterizedText((seconds > 1) ? "seconds_plural" : "seconds_singular", TEXT_BANK_MISC, arg);
                    announceBuf += snprintf(announceBuf, sizeof(ttsAnnounceBuf), "%s", translation.c_str());
                }
                assert(announceBuf < ttsAnnounceBuf + sizeof(ttsAnnounceBuf));
                SpeechSynthesizerSpeak(ttsAnnounceBuf, GetLanguageCode());
                prevTimer = timer;
            }
        }
        
        prevTimer = timer;
        
        if (!GameInteractor::IsSaveLoaded(true)) return;
        
        static int16_t lostHealth = 0;
        static int16_t prevHealth = 0;
        
        if (gSaveContext.health - prevHealth < 0) {
            lostHealth += prevHealth - gSaveContext.health;
        }
        
        if (gPlayState->state.frames % 7 == 0) {
            if (lostHealth >= 16) {
                Audio_PlaySoundGeneral(NA_SE_SY_CANCEL, &D_801333D4, 4, &D_801333E0, &D_801333E0, &D_801333E8);
                lostHealth -= 16;
            }
        }
        
        prevHealth = gSaveContext.health;
    });
}


void RegisterOnKaleidoscopeUpdateHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnKaleidoscopeUpdate>([](int16_t inDungeonScene) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;

        static int16_t prevCursorIndex = 0;
        static uint16_t prevCursorSpecialPos = 0;
        static uint16_t prevCursorPoint[5] = { 0 };
        static int16_t prevPromptChoice = -1;
        static int16_t prevSubState = -1;
        static int16_t prevState = -1;

        PauseContext* pauseCtx = &gPlayState->pauseCtx;
        Input* input = &gPlayState->state.input[0];

        // Save game prompt
        if (pauseCtx->state == 7) {
            if (pauseCtx->unk_1EC == 1) {
                // prompt
                if (prevPromptChoice != pauseCtx->promptChoice) {
                    auto prompt = GetParameterizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                    if (prevPromptChoice == -1) {
                        auto translation = GetParameterizedText("save_prompt", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizerSpeak((translation + " - " + prompt), GetLanguageCode());
                    } else {
                        SpeechSynthesizerSpeak(prompt, GetLanguageCode());
                    }

                    prevPromptChoice = pauseCtx->promptChoice;
                }
            } else if (pauseCtx->unk_1EC == 4 && prevSubState != 4) {
                // Saved
                auto translation = GetParameterizedText("game_saved", TEXT_BANK_KALEIDO, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
            }
            prevSubState = pauseCtx->unk_1EC;
            prevState = pauseCtx->state;
            return;
        }

        // Game over + prompts
        if (pauseCtx->state >= 0xC && pauseCtx->state <= 0x10) {
            // Reset prompt tracker after state change
            if (prevState != pauseCtx->state) {
                prevPromptChoice = -1;
            }

            switch (pauseCtx->state) {
                // Game over in full alpha
                case 0xC: {
                    // Fire once on state change
                    if (prevState != pauseCtx->state) {
                        auto translation = GetParameterizedText("game_over", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    }
                    break;
                }
                // Prompt for save
                case 0xE: {
                    if (prevPromptChoice != pauseCtx->promptChoice) {
                        auto prompt = GetParameterizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                        if (prevPromptChoice == -1) {
                            auto translation = GetParameterizedText("save_prompt", TEXT_BANK_KALEIDO, nullptr);
                            SpeechSynthesizerSpeak(translation + " - " + prompt, GetLanguageCode());
                        } else {
                            SpeechSynthesizerSpeak(prompt, GetLanguageCode());
                        }

                        prevPromptChoice = pauseCtx->promptChoice;
                    }
                    break;
                }
                // Game saved
                case 0xF: {
                    // Fire once on state change
                    if (prevState != pauseCtx->state) {
                        auto translation = GetParameterizedText("game_saved", TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    }
                    break;
                }
                // Prompt to continue playing
                case 0x10: {
                    if (prevPromptChoice != pauseCtx->promptChoice) {
                        auto prompt = GetParameterizedText(pauseCtx->promptChoice == 0 ? "yes" : "no", TEXT_BANK_MISC, nullptr);
                        if (prevPromptChoice == -1) {
                            auto translation = GetParameterizedText("continue_game", TEXT_BANK_KALEIDO, nullptr);
                            SpeechSynthesizerSpeak(translation + " - " + prompt, GetLanguageCode());
                        } else {
                            SpeechSynthesizerSpeak(prompt, GetLanguageCode());
                        }

                        prevPromptChoice = pauseCtx->promptChoice;
                    }
                    break;
                }
            }

            prevState = pauseCtx->state;
            return;
        }

        // Announce page when
        // Kaleido pages are rotating and page halfway rotated
        // Or Kaleido was just opened
        if ((pauseCtx->unk_1E4 == 1 && pauseCtx->unk_1EA == 32) || (pauseCtx->state == 4 && prevState != 4)) {
            uint16_t modeNextPageMap[] = {
                PAUSE_MAP, PAUSE_EQUIP, PAUSE_QUEST, PAUSE_ITEM, PAUSE_EQUIP, PAUSE_MAP, PAUSE_ITEM, PAUSE_QUEST,
            };
            uint16_t nextPage = modeNextPageMap[pauseCtx->mode];

            switch (nextPage) {
                case PAUSE_ITEM: {
                    auto translation = GetParameterizedText("item_menu", TEXT_BANK_KALEIDO, nullptr);
                    SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    break;
                }
                case PAUSE_MAP: {
                    std::string map;
                    if (inDungeonScene) {
                        std::string key = std::to_string(gSaveContext.mapIndex);
                        map = GetParameterizedText(key, TEXT_BANK_SCENES, nullptr);
                    } else {
                        map = GetParameterizedText("overworld", TEXT_BANK_KALEIDO, nullptr);
                    }
                    auto translation = GetParameterizedText("map_menu", TEXT_BANK_KALEIDO, map.c_str());
                    SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    break;
                }
                case PAUSE_QUEST: {
                    auto translation = GetParameterizedText("quest_menu", TEXT_BANK_KALEIDO, nullptr);
                    SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    break;
                }
                case PAUSE_EQUIP: {
                    auto translation = GetParameterizedText("equip_menu", TEXT_BANK_KALEIDO, nullptr);
                    SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    break;
                }
            }
            prevState = pauseCtx->state;
            return;
        }

        prevState = pauseCtx->state;

        if (pauseCtx->state != 6) {
            // Reset cursor index and values so it is announced when pause is reopened
            prevCursorIndex = -1;
            prevPromptChoice = -1;
            prevSubState = -1;
            return;
        }

        if ((pauseCtx->debugState != 1) && (pauseCtx->debugState != 2)) {
            char arg[8];
            if (CHECK_BTN_ALL(input->press.button, BTN_DUP)) {
                // Normalize hearts to fractional count similar to z_lifemeter
                int curHeartFraction = gSaveContext.health % 16;
                int fullHearts = gSaveContext.health / 16;
                float fraction = ceilf((float)curHeartFraction / 5) * 0.25;
                float health = (float)fullHearts + fraction;
                snprintf(arg, sizeof(arg), "%g", health);
                auto translation = GetParameterizedText("health", TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DLEFT) && gSaveContext.magicCapacity != 0) {
                // Normalize magic to percentage
                float magicLevel = ((float)gSaveContext.magic / gSaveContext.magicCapacity) * 100;
                snprintf(arg, sizeof(arg), "%.0f%%", magicLevel);
                auto translation = GetParameterizedText("magic", TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DDOWN)) {
                snprintf(arg, sizeof(arg), "%d", gSaveContext.rupees);
                auto translation = GetParameterizedText("rupees", TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
            } else if (CHECK_BTN_ALL(input->press.button, BTN_DRIGHT)) {
                //TODO: announce timer?
            }
        }
        
        uint16_t cursorIndex = (pauseCtx->pageIndex == PAUSE_MAP && !inDungeonScene) ? PAUSE_WORLD_MAP : pauseCtx->pageIndex;
        if (prevCursorIndex == cursorIndex &&
            prevCursorSpecialPos == pauseCtx->cursorSpecialPos &&
            prevCursorPoint[cursorIndex] == pauseCtx->cursorPoint[cursorIndex]) {
            return;
        }
        
        prevCursorSpecialPos = pauseCtx->cursorSpecialPos;
        
        if (pauseCtx->cursorSpecialPos > 0) {
            return;
        }

        std::string buttonNames[] = {
            "input_button_c_left",
            "input_button_c_down",
            "input_button_c_right",
            "input_d_pad_up",
            "input_d_pad_down",
            "input_d_pad_left",
            "input_d_pad_right",
        };
        int8_t assignedTo = -1;
        
        switch (pauseCtx->pageIndex) {
            case PAUSE_ITEM:
            {
                char arg[8]; // at least big enough where no s8 string will overflow
                switch (pauseCtx->cursorItem[PAUSE_ITEM]) {
                    case ITEM_STICK:
                    case ITEM_NUT:
                    case ITEM_BOMB:
                    case ITEM_BOMBCHU:
                    case ITEM_SLINGSHOT:
                    case ITEM_BOW:
                    case ITEM_BEAN:
                        snprintf(arg, sizeof(arg), "%d", AMMO(pauseCtx->cursorItem[PAUSE_ITEM]));
                        break;
                    default:
                        arg[0] = '\0';
                }

                if (pauseCtx->cursorItem[PAUSE_ITEM] == PAUSE_ITEM_NONE ||
                    pauseCtx->cursorItem[PAUSE_ITEM] == ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_ITEM]);
                std::string itemTranslation = GetParameterizedText(key, TEXT_BANK_KALEIDO, arg);

                // Check if item is assigned to a button
                for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); i++) {
                    if (gSaveContext.equips.buttonItems[i + 1] == pauseCtx->cursorItem[PAUSE_ITEM]) {
                        assignedTo = i;
                        break;
                    }
                }

                if (assignedTo != -1) {
                    auto button = GetParameterizedText(buttonNames[assignedTo], TEXT_BANK_MISC, nullptr);
                    auto translation = GetParameterizedText("assigned_to", TEXT_BANK_KALEIDO, button.c_str());
                    SpeechSynthesizerSpeak(itemTranslation + " - " + translation, GetLanguageCode());
                } else {
                    SpeechSynthesizerSpeak(itemTranslation, GetLanguageCode());
                }
                break;
            }
            case PAUSE_MAP:
                if (inDungeonScene) {
                    // Dungeon map items
                    if (pauseCtx->cursorItem[PAUSE_MAP] != PAUSE_ITEM_NONE) {
                        std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_MAP]);
                        auto translation = GetParameterizedText(key, TEXT_BANK_KALEIDO, nullptr);
                        SpeechSynthesizerSpeak(translation, GetLanguageCode());
                    } else {
                        // Dungeon map floor numbers
                        char arg[8];
                        int cursorPoint = pauseCtx->cursorPoint[PAUSE_MAP];

                        // Cursor is on a dungeon floor position
                        if (cursorPoint >= 3 && cursorPoint < 11) {
                            int floorID = gMapData->floorID[gPlayState->interfaceCtx.unk_25A][pauseCtx->dungeonMapSlot - 3];
                            // Normalize so F1 == 0, and negative numbers are basement levels
                            int normalizedFloor = (floorID * -1) + 8;
                            if (normalizedFloor >= 0) {
                                snprintf(arg, sizeof(arg), "%d", normalizedFloor + 1);
                                auto translation = GetParameterizedText("floor", TEXT_BANK_KALEIDO, arg);
                                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                            } else {
                                snprintf(arg, sizeof(arg), "%d", normalizedFloor * -1);
                                auto translation = GetParameterizedText("basement", TEXT_BANK_KALEIDO, arg);
                                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                            }
                        }
                    }
                } else {
                    std::string key = std::to_string(0x0100 + pauseCtx->cursorPoint[PAUSE_WORLD_MAP]);
                    auto translation = GetParameterizedText(key, TEXT_BANK_KALEIDO, nullptr);
                    SpeechSynthesizerSpeak(translation, GetLanguageCode());
                }
                break;
            case PAUSE_QUEST:
            {
                char arg[8]; // at least big enough where no s8 string will overflow
                switch (pauseCtx->cursorItem[PAUSE_QUEST]) {
                    case ITEM_SKULL_TOKEN:
                        snprintf(arg, sizeof(arg), "%d", gSaveContext.inventory.gsTokens);
                        break;
                    case ITEM_HEART_CONTAINER:
                        snprintf(arg, sizeof(arg), "%d", (gSaveContext.inventory.questItems & 0xF0000000) >> 0x1C);
                        break;
                    default:
                        arg[0] = '\0';
                }

                if (pauseCtx->cursorItem[PAUSE_QUEST] == PAUSE_ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_QUEST]);
                auto translation = GetParameterizedText(key, TEXT_BANK_KALEIDO, arg);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case PAUSE_EQUIP:
            {
                if (pauseCtx->namedItem == PAUSE_ITEM_NONE) {
                    prevCursorIndex = -1;
                    return;
                }

                std::string key = std::to_string(pauseCtx->cursorItem[PAUSE_EQUIP]);
                auto itemTranslation = GetParameterizedText(key, TEXT_BANK_KALEIDO, nullptr);
                uint8_t checkEquipItem = pauseCtx->namedItem;

                // BGS from kaleido reports as ITEM_HEART_PIECE_2 (122)
                // remap BGS and broken knife to be the BGS item for the current equip check
                if (checkEquipItem == ITEM_HEART_PIECE_2 || checkEquipItem == ITEM_SWORD_KNIFE) {
                    checkEquipItem = ITEM_SWORD_BGS;
                }

                // Check if equipment item is currently equipped or assigned to a button
                if (checkEquipItem >= ITEM_SWORD_KOKIRI && checkEquipItem <= ITEM_BOOTS_HOVER) {
                    uint8_t checkEquipType = (checkEquipItem - ITEM_SWORD_KOKIRI) / 3;
                    uint8_t checkEquipValue = ((checkEquipItem - ITEM_SWORD_KOKIRI) % 3) + 1;

                    if (CUR_EQUIP_VALUE(checkEquipType) == checkEquipValue) {
                        itemTranslation = GetParameterizedText("equipped", TEXT_BANK_KALEIDO, itemTranslation.c_str());
                    }

                    for (size_t i = 0; i < ARRAY_COUNT(gSaveContext.equips.cButtonSlots); i++) {
                        if (gSaveContext.equips.buttonItems[i + 1] == checkEquipItem) {
                            assignedTo = i;
                            break;
                        }
                    }
                }

                if (assignedTo != -1) {
                    auto button = GetParameterizedText(buttonNames[assignedTo], TEXT_BANK_MISC, nullptr);
                    auto translation = GetParameterizedText("assigned_to", TEXT_BANK_KALEIDO, button.c_str());
                    SpeechSynthesizerSpeak((itemTranslation + " - " + translation), GetLanguageCode());
                } else {
                    SpeechSynthesizerSpeak(itemTranslation, GetLanguageCode());
                }
                break;
            }
            default:
                break;
        }

        prevCursorIndex = cursorIndex;
        memcpy(prevCursorPoint, pauseCtx->cursorPoint, sizeof(prevCursorPoint));
    });
}

void RegisterOnUpdateMainMenuSelection() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnPresentFileSelect>([]() {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        auto translation = GetParameterizedText("file1", TEXT_BANK_FILECHOOSE, nullptr);
        SpeechSynthesizerSpeak(translation, GetLanguageCode());
    });
    
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileSelectSelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_BTN_MAIN_FILE_1: {
                auto translation = GetParameterizedText("file1", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_MAIN_FILE_2: {
                auto translation = GetParameterizedText("file2", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_MAIN_FILE_3: {
                auto translation = GetParameterizedText("file3", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_MAIN_OPTIONS: {
                auto translation = GetParameterizedText("options", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_MAIN_COPY: {
                auto translation = GetParameterizedText("copy", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_MAIN_ERASE: {
                auto translation = GetParameterizedText("erase", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileSelectConfirmationSelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;

        switch (optionIndex) {
            case FS_BTN_CONFIRM_YES: {
                auto translation = GetParameterizedText("confirm", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_CONFIRM_QUIT: {
                auto translation = GetParameterizedText("quit", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileCopySelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_BTN_COPY_FILE_1: {
                auto translation = GetParameterizedText("file1", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_COPY_FILE_2: {
                auto translation = GetParameterizedText("file2", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_COPY_FILE_3: {
                auto translation = GetParameterizedText("file3", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_COPY_QUIT: {
                auto translation = GetParameterizedText("quit", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });
    
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileCopyConfirmationSelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_BTN_CONFIRM_YES: {
                auto translation = GetParameterizedText("confirm", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_CONFIRM_QUIT: {
                auto translation = GetParameterizedText("quit", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });
    
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileEraseSelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_BTN_ERASE_FILE_1: {
                auto translation = GetParameterizedText("file1", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_ERASE_FILE_2: {
                auto translation = GetParameterizedText("file2", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_ERASE_FILE_3: {
                auto translation = GetParameterizedText("file3", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_ERASE_QUIT: {
                auto translation = GetParameterizedText("quit", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });
    
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileEraseConfirmationSelection>([](uint16_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_BTN_CONFIRM_YES: {
                auto translation = GetParameterizedText("confirm", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_BTN_CONFIRM_QUIT: {
                auto translation = GetParameterizedText("quit", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });
    
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileAudioSelection>([](uint8_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_AUDIO_STEREO: {
                auto translation = GetParameterizedText("audio_stereo", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_AUDIO_MONO: {
                auto translation = GetParameterizedText("audio_mono", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_AUDIO_HEADSET: {
                auto translation = GetParameterizedText("audio_headset", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_AUDIO_SURROUND: {
                auto translation = GetParameterizedText("audio_surround", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileTargetSelection>([](uint8_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case FS_TARGET_SWITCH: {
                auto translation = GetParameterizedText("target_switch", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case FS_TARGET_HOLD: {
                auto translation = GetParameterizedText("target_hold", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileLanguageSelection>([](uint8_t optionIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        switch (optionIndex) {
            case LANGUAGE_ENG: {
                auto translation = GetParameterizedText("language_english", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case LANGUAGE_GER: {
                auto translation = GetParameterizedText("language_german", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case LANGUAGE_FRA: {
                auto translation = GetParameterizedText("language_french", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileQuestSelection>([](uint8_t questIndex) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;

        switch (questIndex) {
            case QUEST_NORMAL: {
                auto translation = GetParameterizedText("quest_sel_vanilla", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case QUEST_MASTER: {
                auto translation = GetParameterizedText("quest_sel_mq", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case QUEST_RANDOMIZER: {
                auto translation = GetParameterizedText("quest_sel_randomizer", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            case QUEST_BOSSRUSH: {
                auto translation = GetParameterizedText("quest_sel_boss_rush", TEXT_BANK_FILECHOOSE, nullptr);
                SpeechSynthesizerSpeak(translation, GetLanguageCode());
                break;
            }
            default:
                break;
        }
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileBossRushOptionSelection>([](uint8_t optionIndex, uint8_t optionValue) {
        if (!CVarGetInteger("gA11yTTS", 0)) return;

        auto optionName = BossRush_GetSettingName(optionIndex, gSaveContext.language);
        auto optionValueName = BossRush_GetSettingChoiceName(optionIndex, optionValue, gSaveContext.language);
        auto translation = optionName + std::string(" - ") + optionValueName;
        SpeechSynthesizerSpeak(translation, GetLanguageCode());
    });

    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnUpdateFileNameSelection>([](int16_t charCode) {
        if (!CVarGetInteger("gA11yTTS", 0))
            return;

        std::string translation;

        if (charCode < 10) { // Digits
            translation = std::to_string(charCode);
        } else if (charCode >= 10 && charCode < 36) { // Uppercase letters
            translation = GetParameterizedText("capital_letter", TEXT_BANK_FILECHOOSE, std::string(1, charCode + 0x37).c_str());
        } else if (charCode >= 36 && charCode < 62) { // Lowercase letters
            translation = std::string(1, charCode + 0x3D);
        } else if (charCode == 62) { // Space
            translation = GetParameterizedText("space", TEXT_BANK_FILECHOOSE, nullptr);
        } else if (charCode == 63) { // -
            translation = GetParameterizedText("hyphen", TEXT_BANK_FILECHOOSE, nullptr);
        } else if (charCode == 64) { // .
            translation = GetParameterizedText("period", TEXT_BANK_FILECHOOSE, nullptr);
        } else if (charCode == 0xF0 + FS_KBD_BTN_BACKSPACE) {
            translation = GetParameterizedText("backspace", TEXT_BANK_FILECHOOSE, nullptr);
        } else if (charCode == 0xF0 + FS_KBD_BTN_END) {
            translation = GetParameterizedText("end", TEXT_BANK_FILECHOOSE, nullptr);
        } else {
            translation = std::string(1, charCode);
        }

        SpeechSynthesizerSpeak(translation, GetLanguageCode());
    });
}

// MARK: - Dialog Messages

static uint8_t ttsHasMessage;
static uint8_t ttsHasNewMessage;
static int8_t ttsCurrentHighlightedChoice;
static std::string msgOutput;

std::string Message_TTS_Decode(uint8_t* sourceBuf, uint16_t startOffset, uint16_t size) {
    std::string result;
    uint8_t isListingChoices = 0;

    for (uint16_t i = 0; i < size; i++) {
        uint8_t cchar = sourceBuf[i + startOffset];

        if (cchar == MESSAGE_NEWLINE) {
            result += (isListingChoices) ? '\n' : ' ';
        } else if (cchar == MESSAGE_THREE_CHOICE || cchar == MESSAGE_TWO_CHOICE) {
            result += '\n';
            isListingChoices = 1;
        } else if (cchar == MESSAGE_COLOR || cchar == MESSAGE_SHIFT || cchar == MESSAGE_TEXT_SPEED ||
                   cchar == MESSAGE_BOX_BREAK_DELAYED || cchar == MESSAGE_FADE || cchar == MESSAGE_ITEM_ICON) {
            i++;
        } else if (cchar == MESSAGE_FADE2 || cchar == MESSAGE_SFX || cchar == MESSAGE_TEXTID) {
            i += 2;
        } else if (cchar == 0x1A || cchar == 0x08) { // skip
        } else {
            auto it = charReplacements.find(cchar);
            if (it != charReplacements.end()) {
                result += it->second;
            } else {
                result += cchar;
            }
        }
    }
    return result;
}

void RegisterOnDialogMessageHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnDialogMessage>([]() {
        if (!CVarGetInteger("gA11yTTS", 0)) return;
        
        MessageContext *msgCtx = &gPlayState->msgCtx;
        Actor *msgTlk = msgCtx->talkActor;
        s16 actorID = 0;
        s16 actorArg = 0;
        if (msgTlk != nullptr) {
            actorID = msgTlk->id;
            actorArg = msgTlk->params;
            if (actorArg < 0)
                actorArg += 256;
        }
        
        if (msgCtx->msgMode == MSGMODE_TEXT_NEXT_MSG || msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED_TEXT_BEGIN || (msgCtx->msgMode == MSGMODE_TEXT_CONTINUING && msgCtx->stateTimer == 1)) {
            ttsHasNewMessage = 1;
        } else if (msgCtx->msgMode == MSGMODE_TEXT_DISPLAYING || msgCtx->msgMode == MSGMODE_TEXT_AWAIT_NEXT || msgCtx->msgMode == MSGMODE_TEXT_DONE || msgCtx->msgMode == MSGMODE_TEXT_DELAYED_BREAK
                   || msgCtx->msgMode == MSGMODE_OCARINA_STARTING || msgCtx->msgMode == MSGMODE_OCARINA_PLAYING
                   || msgCtx->msgMode == MSGMODE_DISPLAY_SONG_PLAYED_TEXT || msgCtx->msgMode == MSGMODE_SONG_PLAYED_ACT_BEGIN || msgCtx->msgMode == MSGMODE_SONG_PLAYED_ACT || msgCtx->msgMode == MSGMODE_SONG_PLAYBACK_STARTING || msgCtx->msgMode == MSGMODE_SONG_PLAYBACK || msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_STARTING || msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION_SELECT_INSTRUMENT || msgCtx->msgMode == MSGMODE_SONG_DEMONSTRATION
        ) {
            if (ttsHasNewMessage) {
                ttsHasMessage = 1;
                ttsHasNewMessage = 0;
                ttsCurrentHighlightedChoice = 0;
                
                uint16_t size = msgCtx->decodedTextLen;
                msgOutput = Message_TTS_Decode(msgCtx->msgBufDecoded, 0, size);
                const char* langCode = GetLanguageCode();
                bool langIsES = (strcmp(langCode, "es-ES") == 0);
                const char* langSpoken;
                std::string gender = "male";
                if (langIsES)
                    langSpoken = "es-ES_tradnl";
                else
                    langSpoken = langCode;

                osSyncPrintf("ACTOR: id %d, arg '%d'\n", actorID, actorArg);

                if (actorID == ACTOR_EN_SA) // Saria
                    gender = "female";
                else if (actorID == ACTOR_EN_ELF || actorID == ACTOR_EN_TITE || actorID == ACTOR_ELF_MSG || actorID == ACTOR_BOSS_MO) // Navi
                {
                    gender = "female";
                    if (langIsES)
                        langSpoken = "es-CO";
                }
                else if (actorID == ACTOR_EN_KO) // Kokiri Kids
                    if (actorArg == 6 || actorArg == 1 || actorArg == 12)
                        gender = "female";
                    else
                        if (langIsES)
                            langSpoken = "es-AR";

                std::string pfx = "<voice gender=\"" + gender + "\" languages=\"" + langSpoken +
                                  "\" required=\"languages gender variant\">" + msgOutput + "</voice>";

                osSyncPrintf("MSG:'%s'\n", pfx.c_str());
                SpeechSynthesizerSpeak(pfx, GetLanguageCode()); // previously langSpoken, for some reason didn't work last time
            } else if (msgCtx->msgMode == MSGMODE_TEXT_DONE && msgCtx->choiceNum > 0 && msgCtx->choiceIndex != ttsCurrentHighlightedChoice) {
                ttsCurrentHighlightedChoice = msgCtx->choiceIndex;
                uint16_t startOffset = 0;
                while (startOffset < msgCtx->decodedTextLen) {
                    if (msgCtx->msgBufDecoded[startOffset] == MESSAGE_TWO_CHOICE || msgCtx->msgBufDecoded[startOffset] == MESSAGE_THREE_CHOICE) {
                        startOffset++;
                        break;
                    }
                    startOffset++;
                }
                
                uint16_t endOffset = 0;
                if (startOffset < msgCtx->decodedTextLen) {
                    uint8_t i = msgCtx->choiceIndex;
                    while (i-- > 0) {
                        while (startOffset < msgCtx->decodedTextLen) {
                            if (msgCtx->msgBufDecoded[startOffset] == MESSAGE_NEWLINE) {
                                startOffset++;
                                break;
                            }
                            startOffset++;
                        }
                    }
                    
                    endOffset = startOffset;
                    while (endOffset < msgCtx->decodedTextLen) {
                        if (msgCtx->msgBufDecoded[endOffset] == MESSAGE_NEWLINE) {
                            break;
                        }
                        endOffset++;
                    }
                    
                    if (startOffset < msgCtx->decodedTextLen && startOffset != endOffset) {
                        uint16_t size = endOffset - startOffset;
                        msgOutput = Message_TTS_Decode(msgCtx->msgBufDecoded, startOffset, size);
                        SpeechSynthesizerSpeak(msgOutput, GetLanguageCode());
                    }
                }
            }
        } else if (ttsHasMessage) {
            ttsHasMessage = 0;
            ttsHasNewMessage = 0;
            
            if ((msgCtx->decodedTextLen < 3 ||
                (msgCtx->msgBufDecoded[msgCtx->decodedTextLen - 2] != MESSAGE_FADE && msgCtx->msgBufDecoded[msgCtx->decodedTextLen - 3] != MESSAGE_FADE2)) && msgCtx->ocarinaMode == OCARINA_MODE_00) {
                    SpeechSynthesizerSpeak("", GetLanguageCode()); // cancel current speech (except for faded out messages and ocarina played songs)
            }
        }
    });
}

// MARK: - Main Registration

void InitTTSBank() {
    LoadAccessibilityTexts(GetJSONLanguage());
}

void RegisterOnSetGameLanguageHook() {
    GameInteractor::Instance->RegisterGameHook<GameInteractor::OnSetGameLanguage>([]() {
        InitTTSBank();
    });
}

void RegisterTTSModHooks() {
    RegisterOnSetGameLanguageHook();
    RegisterOnDialogMessageHook();
    RegisterOnSceneInitHook();
    RegisterOnPresentTitleCardHook();
    RegisterOnInterfaceUpdateHook();
    RegisterOnKaleidoscopeUpdateHook();
    RegisterOnUpdateMainMenuSelection();
}

void RegisterTTS() {
    InitTTSBank();
    RegisterTTSModHooks();
}

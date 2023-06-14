#include <soh/OTRGlobals.h>

extern "C" {
#include <variables.h>
}

// RANDOTODO: Port the rest of the behavior associated with this option here.

void BuildShopDescMessage(uint16_t* textId, bool* loadFromMessageTable) {
    CustomMessage msg =
        CustomMessage("\x08%rBombchu  (10 piezas)  99 Rupias&%wParece un ratôn de juguete,&¡pero es una bomba de "
                      "tiempo&auto-propulsada!\x09\x0A",
                      "\x08%rKrabbelmine  10 Stück  99 Rubine&%wDas ist eine praktische Zeitbombe,&die Du als "
                      "Distanzwaffe&einsetzen kannst!\x09\x0A",
                      "\x08%rMissile  10 unités  99 Rubis&%wProfilée comme une souris&mécanique, cette arme est "
                      "&destructrice!!!\x09\x0A");
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BuildShopPromptMessage(uint16_t* textId, bool* loadFromMessageTable) {
    CustomMessage msg = CustomMessage("\x08Bombchu  10 piezas   99 Rupias\x09&&\x1B%gSï&No comprar%w",
                                      "\010Krabbelmine  10 Stück  99 Rubine\x09&&\x1B%gKaufen!&Nicht kaufen!%w",
                                      "\010Missiles  10 unités   99 Rubis\x09&&\x1B%gAcheter&Ne pas acheter%w");
    msg.AutoFormat();
    msg.LoadIntoFont();
    *loadFromMessageTable = false;
}

void BetterBombchuShopping_Register() {
    COND_ID_HOOK(OnOpenText, TEXT_BUY_BOMBCHUS_10_DESC,
                 IS_RANDO || CVarGetInteger(CVAR_ENHANCEMENT("BetterBombchuShopping"), 0), BuildShopDescMessage);
    COND_ID_HOOK(OnOpenText, TEXT_BUY_BOMBCHUS_10_PROMPT,
                 IS_RANDO || CVarGetInteger(CVAR_ENHANCEMENT("BetterBombchuShopping"), 0), BuildShopPromptMessage);
}

static RegisterShipInitFunc initFunc(BetterBombchuShopping_Register,
                                     { "IS_RANDO", CVAR_ENHANCEMENT("BetterBombchuShopping") });

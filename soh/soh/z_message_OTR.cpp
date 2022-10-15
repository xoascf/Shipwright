#include "OTRGlobals.h"
#include <ResourceMgr.h>
#include <Scene.h>
#include <Utils/StringHelper.h>
#include "global.h"
#include "vt.h"
#include <Text.h>
#include <message_data_static.h>
#include "Enhancements/custom-message/CustomMessageManager.h"
#include "Enhancements/custom-message/CustomMessageTypes.h"

extern "C" MessageTableEntry* sNesMessageEntryTablePtr;
extern "C" MessageTableEntry* sGerMessageEntryTablePtr;
extern "C" MessageTableEntry* sFraMessageEntryTablePtr;
extern "C" MessageTableEntry* sStaffMessageEntryTablePtr;
//extern "C" MessageTableEntry* _message_0xFFFC_nes;	

MessageTableEntry* OTRMessage_LoadTable(const char* filePath, bool isNES) {
    auto file = std::static_pointer_cast<Ship::Text>(OTRGlobals::Instance->context->GetResourceManager()->LoadResource(filePath));

    if (file == nullptr)
        return nullptr;
    
    // Allocate room for an additional message
    MessageTableEntry* table = (MessageTableEntry*)malloc(sizeof(MessageTableEntry) * (file->messages.size() + 1));

    for (int i = 0; i < file->messages.size(); i++) {
        // Look for Owl Text
        if (file->messages[i].id == 0x2066) {
            // Create a new message based on the Owl Text
            uint32_t kaeporaMsgSize = file->messages[i].msg.size();
            char* kaeporaOg = (char*)malloc(sizeof(char) * kaeporaMsgSize);
            char* kaeporaPatch = (char*)malloc(sizeof(char) * kaeporaMsgSize);
            file->messages[i].msg.copy(kaeporaOg, kaeporaMsgSize, 0);
            file->messages[i].msg.copy(kaeporaPatch, kaeporaMsgSize, 0);

            size_t colorPos = file->messages[i].msg.find(QM_GREEN);
            size_t newLinePos = colorPos + file->messages[i].msg.substr(colorPos + 1).find(CTRL_NEWLINE) + 1;
            size_t endColorPos = newLinePos + file->messages[i].msg.substr(newLinePos).find(CTRL_COLOR);
            size_t NoLength = newLinePos - (colorPos + 1);
            size_t YesLength = endColorPos - (newLinePos + 1);
            // Swap the order of yes and no in this new message
            size_t yes = 0;
            while (yes < YesLength) {
                kaeporaPatch[colorPos + yes + 1] = kaeporaOg[newLinePos + yes + 1];
                yes++;
            }
            kaeporaPatch[colorPos + yes + 1] = CTRL_NEWLINE;
            size_t no = 0;
            while (no < NoLength) {
                kaeporaPatch[colorPos + yes + 2 + no] = kaeporaOg[colorPos + 1 + no];
                no++;
            }

            // load data into message
            table[file->messages.size()].textId = 0x71B3;
            table[file->messages.size()].typePos = (file->messages[i].textboxType << 4) | file->messages[i].textboxYPos;
            table[file->messages.size()].segment = kaeporaPatch;
            table[file->messages.size()].msgSize = kaeporaMsgSize;
        }

        table[i].textId = file->messages[i].id;
        table[i].typePos = (file->messages[i].textboxType << 4) | file->messages[i].textboxYPos;
        table[i].segment = file->messages[i].msg.c_str();
        table[i].msgSize = file->messages[i].msg.size();

        if (isNES && file->messages[i].id == 0xFFFC)
            _message_0xFFFC_nes = (char*)file->messages[i].msg.c_str();
    }

	return table;
}

extern "C" void OTRMessage_Init()
{
    sNesMessageEntryTablePtr = OTRMessage_LoadTable("text/nes_message_data_static/nes_message_data_static", true);
    sGerMessageEntryTablePtr = OTRMessage_LoadTable("text/ger_message_data_static/ger_message_data_static", false);
    sFraMessageEntryTablePtr = OTRMessage_LoadTable("text/fra_message_data_static/fra_message_data_static", false);

	auto file2 = std::static_pointer_cast<Ship::Text>(OTRGlobals::Instance->context->GetResourceManager()->LoadResource("text/staff_message_data_static/staff_message_data_static"));

	sStaffMessageEntryTablePtr = (MessageTableEntry*)malloc(sizeof(MessageTableEntry) * file2->messages.size());

	for (int i = 0; i < file2->messages.size(); i++)
	{
		sStaffMessageEntryTablePtr[i].textId = file2->messages[i].id;
		sStaffMessageEntryTablePtr[i].typePos = (file2->messages[i].textboxType << 4) | file2->messages[i].textboxYPos;
		sStaffMessageEntryTablePtr[i].segment = file2->messages[i].msg.c_str();
		sStaffMessageEntryTablePtr[i].msgSize = file2->messages[i].msg.size();
	}

    CustomMessageManager::Instance->AddCustomMessageTable(customMessageTableID);
    CustomMessageManager::Instance->AddCustomMessageTable(questMessageTableID);
    CustomMessageManager::Instance->CreateGetItemMessage(
        customMessageTableID, (GetItemID)TEXT_GS_NO_FREEZE, ITEM_SKULL_TOKEN,
        { 
            TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
            "You got a %rGold Skulltula Token%w!&You've collected %r{{gsCount}}%w tokens&in total!\x0E\x3C",
            "Ein %rGoldenes Skulltula-Symbol%w!&Du hast nun insgesamt %r{{gsCount}}%w Golende&Skulltula-Symbole gesammelt!\x0E\x3C",
            "Vous obtenez un %rSymbole de&Skulltula d'or%w! Vous avez&collecté %r{{gsCount}}%w symboles en tout!\x0E\x3C"
        }
    );
    CustomMessageManager::Instance->CreateGetItemMessage(
        customMessageTableID, (GetItemID)TEXT_GS_FREEZE, ITEM_SKULL_TOKEN,
        { 
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "You got a %rGold Skulltula Token%w!&You've collected %r{{gsCount}}%w tokens&in total!",
          "Ein %rGoldenes Skulltula-Symbol%w!&Du hast nun insgesamt %r{{gsCount}}%w Golende&Skulltula-Symbole gesammelt!",
          "Vous obtenez un %rSymbole de&Skulltula d'or%w! Vous avez&collecté %r{{gsCount}}%w symboles en tout!"
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        customMessageTableID, TEXT_BUY_BOMBCHU_10_DESC,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "\x08%rBombchu  (10 pieces)  99 Rupees&%wThis looks like a toy mouse, but&it's actually a self-propelled time&bomb!\x09\x0A",
          "\x08%rKrabbelmine  10 Stück  99 Rubine&%wDas ist eine praktische Zeitbombe,&die Du als Distanzwaffe&einsetzen kannst!\x09\x0A",
          "\x08%rMissile  10 unités  99 Rubis&%wProfilée comme une souris&mécanique, cette arme est &destructrice!!!\x09\x0A",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        customMessageTableID, TEXT_BUY_BOMBCHU_10_PROMPT,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "\x08" "Bombchu  10 pieces   99 Rupees\x09&&\x1B%gBuy&Don't buy%w",
          "\x08Krabbelmine  10 Stück  99 Rubine\x09&&\x1B%gKaufen!&Nicht kaufen!%w",
          "\x08Missiles  10 unités   99 Rubis\x09&&\x1B%gAcheter&Ne pas acheter%w",
        }
    );
    CustomMessageManager::Instance->CreateGetItemMessage(
        customMessageTableID, (GetItemID)TEXT_HEART_CONTAINER, ITEM_HEART_CONTAINER,
        {
            TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
            "You got a %rHeart Container%w!&You've collected %r{{heartContainerCount}}%w containers&in total!",
            "Ein %rHerzcontainer%w!&Du hast nun insgesamt %r{{heartContainerCount}}%w&Herzcontainer gesammelt!",
            "Vous obtenez un %rCoeur&d'Energie%w! Vous en avez&collecté %r{{heartContainerCount}}%w en tout!"
        }
    );
    CustomMessageManager::Instance->CreateGetItemMessage(
        customMessageTableID, (GetItemID)TEXT_HEART_PIECE, ITEM_HEART_PIECE,
        {
            TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
            "You got a %rHeart Piece%w!&You've collected %r{{heartPieceCount}}%w pieces&in total!",
            "Ein %rHerzteil%w!&Du hast nun insgesamt %r{{heartPieceCount}}%w&Herteile gesammelt!",
            "Vous obtenez un %rQuart de&Coeur%w! Vous en avez collecté&%r{{heartPieceCount}}%w en tout!"
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        customMessageTableID, TEXT_MARKET_GUARD_NIGHT,
        {
            TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
            "You look bored. Wanna go out for a&walk?\x1B&%gYes&No%w",
            "Du siehst gelangweilt aus.&Willst du einen Spaziergang machen?\x1B&%gJa&Nein%w",
            "Tu as l'air de t'ennuyer. Tu veux&aller faire un tour?\x1B&%gOui&Non%w",
        }
    );
    u16 SariaMsg = TextIDAllocator::Instance->allocateRange("saria", 20);
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "It's nice that you are dropping by the&village again. I've heard the twins have&been arranging something special!\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+1,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "\x0F, you've been collecting a lot of&things in your place, haven't you?&Maybe I should come around sometime.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+2,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "The shopkeeper doesn't do a very&good job of hiding everything in&the store. Tee hee!\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+3,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "I shouldn't be admitting this to you,&but even I sometimes get the twins&confused with the other girls.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+4,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "You probably shouldn't let&Mido know I'm talking to you like this...\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+5,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "Make yourself at home.&I don't mind if other people&clean things up a bit for me.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+6,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "The guys here know a lot of things.&But surely, there has to be so much&more outside the forest to discover.^\x0F, I want you you to teach&me about the world. Talk to me&about any places you discover.^I'm just so curious.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+7,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "I can hear so many people!&You're actually in the&castle town market,&aren't you?^It's just one of those&places I've heard the birds&chattering about sometimes.^Few of them can stand such a&noisy place, but I hear they&can find nice food there!\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+8,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "Hmm... I never would have known&there was a place with&so much water.&It must look amazing!^Come to think of it I do&wonder what it would be&like to be a creture that&could live under water.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+9,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "What a strange place...&So, outside the forest,&those that die still remain,^and so they actually put&them under the ground&to hide them away?\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+10,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "So wow, you are in the place&where the water people live!&I think some of the Kokiri^have actually seen them before,&coming out from that deep&pool in the forest.^Have you met their princess?&I'd love to meet her myself.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+11,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "So, you're at the royal family's&castle, where you snuck&in before right?^Tee hee, if you're feeling a&bit naughty, maybe you could find&a way to get right inside, and&discover what's really going on.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+12,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "So there are places like this&that keep lots of big&creatures around, huh.^That sounds slightly scary.\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+13,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "\x0F, what are all those creatures&making those curious sounds?^There are those ones that make&that loud MUUUOAH sound.&And then, there's something else...^I don't even know if you can&hear it, maybe it's only&my Kokiri ears that can pick&it up, but it's there,^making this incredibly strange&high pitched noise.^What is that?\x0B\x02",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, SariaMsg+14,
        {
          TEXTBOX_TYPE_BLUE, TEXTBOX_POS_BOTTOM,
          "\x0F, what are all those creatures&making those curious sounds?^There are those ones that make&that loud MUUUOAH sound.&There are also those&ones that sometimes huff^and scratch and go NIIIHEHE!&And then, there's something else...^I don't even know if you can&hear it, maybe it's only&my Kokiri ears that can pick&it up, but it's there,^making this incredibly strange&high pitched noise.^What is that?\x0B\x02",
          "",
          "",
        }
    );
    u16 KokiriMsg = TextIDAllocator::Instance->allocateRange("kokiri", 20);
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "Hey, you know, it's so strange what&happened to the Great Deku Tree.^Things haven't been growing&well in the forrest lately, but&hopefully that will change.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+1,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "Tee hee hee, a lot of Kokiri&are good at hiding secrets.^But some aren't as&good as it as others.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+2,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "Hey, wouldn't it be funny, if \x14\x03someone\x14\x01&played a prank on Mido...^Saria is maybe a bit too nice to him&for his own good.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, 0x1074,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "You know, I wish the vegetation&around here had gotten&more water years ago.^If that had happened, there's no doubt&that things would have grown&a mighty lot greater, and the&forest would be a better place for it.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+4,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "As it is, plants are barely&growing at all, and we're&only getting weeds.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+5,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "Things are growing somewhat,&but there are still places&that could be better...",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+6,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "I've spent years tending to&the plants in this forest,&and would you look&at what's happened.^Bountiful foliage all around,&showing what results when&you put in the effort to&make things grow right.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+7,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "You bet I'm proud! In fact,&since I'm feeling so generous,^I'm happy to share some of&the life the forest has in excess.",
          "",
          "",
        }
    );
    static u16 msg8 = KokiriMsg+9;
    static std::string msg8Str = "You know, I think there are places&in the lost woods that no Kokiri&now remembers how to get to...&\x0D\x14\x03...except one...^Oops, I shouldn't have said that...\x07";
    msg8Str.push_back((char)((msg8>>8)&0xFF));
    msg8Str.push_back((char)((msg8)&0xFF));
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+8,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          msg8Str,
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+9,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "This is no longer a secret&to everyone anymore!",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+10,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "I'll give you this, if you&promise to never let anyone know.",
          "",
          "",
        }
    );
    CustomMessageManager::Instance->CreateMessage(
        questMessageTableID, KokiriMsg+11,
        {
          TEXTBOX_TYPE_BLACK, TEXTBOX_POS_BOTTOM,
          "Things are mostly growing&nicely, but I still can't help&but think that maybe just one&spot ought to have grown better...",
          "",
          "",
        }
    );
}

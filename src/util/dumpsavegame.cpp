#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "savegame.h"
#include "names.h"

int verbose = 0;

void showSaveGame(SaveGame *sg);
void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec);
char *itemsString(unsigned short items);

int main(int argc, char *argv[])
{
    SaveGame sg;
    FILE *in;
    if (argc != 2) {
        std::fprintf(stderr, "usage: %s party.sav\n", argv[0]);
        exit(1);
    }
    in = std::fopen(argv[1], "rb");
    if (!in) {
        std::perror(argv[1]);
        std::exit(1);
    }
    sg.read(in);
    showSaveGame(&sg);
    return 0;
}

void showSaveGame(SaveGame *sg)
{
    int i;
    std::printf("???: %x\n", sg->unknown1);
    std::printf(
        "moves: %-4d food: %-5g gold: %d\n",
        sg->moves,
        static_cast<double>(sg->food) / 100.0,
        sg->gold
    );
    std::printf("karma: [ ");
    for (i = 0; i < 8; i++) {
        std::printf("%d ", sg->karma[i]);
    }
    std::printf("]\n");
    std::printf(
        "torches: %-2d gems: %-5d keys: %-5d sextants: %d\n",
        sg->torches,
        sg->gems,
        sg->keys,
        sg->sextants
    );
    std::printf("armor: [ ");
    for (i = 0; i < ARMR_MAX; i++) {
        std::printf("%d ", sg->armor[i]);
    }
    std::printf("]\n");
    std::printf("weapons: [ ");
    for (i = 0; i < WEAP_MAX; i++) {
        std::printf("%d ", sg->weapons[i]);
    }
    std::printf("]\n");
    std::printf("reagents: [ ");
    for (i = 0; i < REAG_MAX; i++) {
        std::printf("%d ", sg->reagents[i]);
    }
    std::printf("]\n");
    std::printf("mixtures: [ ");
    for (i = 0; i < 26; i++) {
        std::printf("%d ", sg->mixtures[i]);
    }
    std::printf("]\n");
    std::printf("items: %s\n", itemsString(sg->items));
    std::printf("x: %-8d y: %d\n", sg->x, sg->y);
    std::printf("stones: %-3x runes %x\n", sg->stones, sg->runes);
    std::printf("party members: %d\n", sg->members);
    std::printf("transport: %x\n", sg->transport);
    std::printf("balloon state/torch duration: %x\n", sg->balloonstate);
    std::printf(
        "trammel: %d  felucca: %d\n", sg->trammelphase, sg->feluccaphase
    );
    std::printf("shiphull: %d\n", sg->shiphull);
    std::printf("lbintro: %d\n", sg->lbintro);
    std::printf(
        "lastcamp: %d       lastreagent: %d\n", sg->lastcamp, sg->lastreagent
    );
    std::printf(
        "lastmeditation: %d lastvirtue: %d\n",
        sg->lastmeditation,
        sg->lastvirtue
    );
    std::printf(
        "dngx: %-5d dngy: %-5d orientation: %d dnglevel: %d\n",
        sg->dngx,
        sg->dngy,
        sg->orientation,
        sg->dnglevel
    );
    std::printf("location: %x\n", sg->location);
    for (i = 0; i < 8; i++) {
        std::printf("player %d\n", i);
        showSaveGamePlayerRecord(&(sg->players[i]));
    }
}

void showSaveGamePlayerRecord(SaveGamePlayerRecord *rec)
{
    static const char *const weapNames[] = {
        "Hands",
        "Staff",
        "Dagger",
        "Sling",
        "Mace",
        "Axe",
        "Sword",
        "Bow",
        "Crossbow",
        "Flaming Oil",
        "Halberd",
        "Magic Axe",
        "Magic Sword",
        "Magic Bow",
        "Magic Wand",
        "Mystic Sword"
    };
    static const char *const armorNames[] = {
        "Skin",
        "Cloth",
        "Leather",
        "Chain Mail",
        "Plate Mail",
        "Magic Chain",
        "Magic Plate",
        "Mystic Robe"
    };
    std::printf(
        "  name: %-17s hp: %-7d hpMax: %-4d xp: %d\n",
        rec->name,
        rec->hp,
        rec->hpMax,
        rec->xp
    );
    std::printf(
        "  str: %-6d dex: %-6d intel: %-4d mp: %-7d ???: %d\n",
        rec->str,
        rec->dex,
        rec->intel,
        rec->mp,
        rec->unknown
    );
    std::printf(
        "  weapon: %-15s armor: %s\n",
        weapNames[rec->weapon],
        armorNames[rec->armor]
    );
    std::printf(
        "  sex: %-6s class: %-16s status: %c\n",
        rec->sex == 11 ? "M" : "F",
        getClassNameEnglish(rec->klass),
        rec->status
    );
}

char *itemsString(unsigned short items)
{
    static char buffer[256];
    int first = 1;
    buffer[0] = '\0';
    if (items & ITEM_SKULL) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_SKULL)
        );
        first = 0;
    }
    if (items & ITEM_SKULL_DESTROYED) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "skull destroyed");
        first = 0;
    }
    if (items & ITEM_CANDLE) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_CANDLE)
        );
        first = 0;
    }
    if (items & ITEM_BOOK) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_BOOK)
        );
        first = 0;
    }
    if (items & ITEM_BELL) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_BELL)
        );
        first = 0;
    }
    if (items & ITEM_KEY_C) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "key c");
        first = 0;
    }
    if (items & ITEM_KEY_L) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "key l");
        first = 0;
    }
    if (items & ITEM_KEY_T) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "key t");
        first = 0;
    }
    if (items & ITEM_HORN) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_HORN)
        );
        first = 0;
    }
    if (items & ITEM_WHEEL) {
        std::strcat(
            std::strcat(buffer, first ? "" : ", "), getItemName(ITEM_WHEEL)
        );
        first = 0;
    }
    if (items & ITEM_CANDLE_USED) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "candle used");
        first = 0;
    }
    if (items & ITEM_BOOK_USED) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "book used");
        first = 0;
    }
    if (items & ITEM_BELL_USED) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "bell used");
        first = 0;
    }
    if (items & 0x2000) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "(bit 14)");
        first = 0;
    }
    if (items & 0x4000) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "(bit 15)");
        first = 0;
    }
    if (items & 0x8000) {
        std::strcat(std::strcat(buffer, first ? "" : ", "), "(bit 16)");
        first = 0;
    }
    return buffer;
}

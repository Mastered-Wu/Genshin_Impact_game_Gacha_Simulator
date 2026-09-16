#pragma once

#include <string>
#include <vector>

namespace gacha {

enum class BannerType { CharacterEvent, WeaponEvent, Standard };
enum class ItemKind { Character, Weapon };

struct Item {
    std::string id;
    std::string name;
    int rarity = 3;
    ItemKind kind = ItemKind::Weapon;
    bool featured = false;
    bool promotional = false;
};

struct BannerConfig {
    std::string id;
    std::string name;
    BannerType type = BannerType::Standard;
    double fiveStarBaseRate = 0.006;
    int fiveStarHardPity = 90;
    double fourStarBaseRate = 0.051;
    int fourStarHardPity = 10;
    std::vector<Item> items;
};

struct WishResult {
    int wishNumber = 0;
    Item item;
    bool hitHardPity5 = false;
    bool hitHardPity4 = false;
    bool usedGuarantee = false;
    int fatePointsAfter = 0;
};

struct WishStats {
    int total = 0;
    int fiveStars = 0;
    int fourStars = 0;
    int featuredFiveStars = 0;
};

struct WishState {
    int pity5 = 0;
    int pity4 = 0;
    bool featuredGuarantee = false;
    bool promotionalGuarantee = false;
    int fatePoints = 0;
    std::string selectedPathItemId;
    std::vector<WishResult> history;
    WishStats stats;
};

struct WishBatch {
    std::vector<WishResult> results;
    WishState state;
};

}

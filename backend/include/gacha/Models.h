#pragma once

#include <string>
#include <vector>

namespace gacha {

// 卡池类型决定 5 星命中后的特殊规则：角色 50/50、武器 75/25、常驻无限定保底。
enum class BannerType { CharacterEvent, WeaponEvent, Standard };

// 物品类型目前只区分角色和武器，方便前端显示与常驻池混合配置。
enum class ItemKind { Character, Weapon };

// 单个可抽物品的元数据。featured 用于角色活动限定，promotional 用于武器活动限定。
struct Item {
    std::string id;
    std::string name;
    int rarity = 3;
    ItemKind kind = ItemKind::Weapon;
    bool featured = false;
    bool promotional = false;
};

// 一个卡池的完整规则配置：基础概率、硬保底阈值和可抽物品列表。
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

// 单次抽卡结果。除了物品本身，也记录是否触发硬保底、限定保底和抽后命定值。
struct WishResult {
    int wishNumber = 0;
    Item item;
    bool hitHardPity5 = false;
    bool hitHardPity4 = false;
    bool usedGuarantee = false;
    int fatePointsAfter = 0;
};

// 当前卡池的汇总统计，供 API 和前端直接展示。
struct WishStats {
    int total = 0;
    int fiveStars = 0;
    int fourStars = 0;
    int featuredFiveStars = 0;
};

// 单个卡池的运行状态。每个卡池独立保存保底、限定保底、命定值、历史和统计。
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

// 一批抽卡的返回值：本批结果和抽完后的状态快照。
struct WishBatch {
    std::vector<WishResult> results;
    WishState state;
};

}

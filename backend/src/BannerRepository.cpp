#include "gacha/BannerRepository.h"

namespace gacha {

// 第一版后端直接使用内置卡池配置，保证离线运行且避免解析依赖。
// configs/banners.json 保留为可审阅的项目资料，字段需要和这里保持一致。
std::map<std::string, BannerConfig> loadBuiltInBanners() {
    BannerConfig character;
    character.id = "character-event";
    character.name = "星辉同行";
    character.type = BannerType::CharacterEvent;
    character.fiveStarBaseRate = 0.006;
    character.fiveStarHardPity = 90;
    character.fourStarBaseRate = 0.051;
    character.fourStarHardPity = 10;
    character.items = {
        {"featured-hero", "星辉旅人", 5, ItemKind::Character, true, false},
        {"standard-hero-a", "晴岚术士", 5, ItemKind::Character, false, false},
        {"standard-hero-b", "霜镜学者", 5, ItemKind::Character, false, false},
        {"featured-four-a", "巡夜弓手", 4, ItemKind::Character, true, false},
        {"featured-four-b", "织焰乐师", 4, ItemKind::Character, true, false},
        {"standard-four-weapon", "铁影大剑", 4, ItemKind::Weapon, false, false},
        {"three-sword", "训练长剑", 3, ItemKind::Weapon, false, false},
        {"three-bow", "旧式猎弓", 3, ItemKind::Weapon, false, false},
        {"three-catalyst", "学徒法器", 3, ItemKind::Weapon, false, false},
    };

    BannerConfig weapon;
    weapon.id = "weapon-event";
    weapon.name = "辉铸兵装";
    weapon.type = BannerType::WeaponEvent;
    weapon.fiveStarBaseRate = 0.007;
    weapon.fiveStarHardPity = 80;
    weapon.fourStarBaseRate = 0.06;
    weapon.fourStarHardPity = 10;
    weapon.items = {
        {"weapon-a", "苍曜长弓", 5, ItemKind::Weapon, false, true},
        {"weapon-b", "赤砂法杖", 5, ItemKind::Weapon, false, true},
        {"weapon-standard-a", "白银阔刃", 5, ItemKind::Weapon, false, false},
        {"weapon-standard-b", "潮音长枪", 5, ItemKind::Weapon, false, false},
        {"weapon-four-a", "鸣砂短剑", 4, ItemKind::Weapon, false, false},
        {"weapon-four-b", "夜航弩", 4, ItemKind::Weapon, false, false},
        {"three-sword", "训练长剑", 3, ItemKind::Weapon, false, false},
        {"three-bow", "旧式猎弓", 3, ItemKind::Weapon, false, false},
        {"three-catalyst", "学徒法器", 3, ItemKind::Weapon, false, false},
    };

    BannerConfig standard;
    standard.id = "standard";
    standard.name = "常驻星轨";
    standard.type = BannerType::Standard;
    standard.fiveStarBaseRate = 0.006;
    standard.fiveStarHardPity = 90;
    standard.fourStarBaseRate = 0.051;
    standard.fourStarHardPity = 10;
    standard.items = {
        {"standard-hero-a", "晴岚术士", 5, ItemKind::Character, false, false},
        {"standard-hero-b", "霜镜学者", 5, ItemKind::Character, false, false},
        {"standard-hero-c", "松庭守卫", 5, ItemKind::Character, false, false},
        {"weapon-standard-a", "白银阔刃", 5, ItemKind::Weapon, false, false},
        {"weapon-standard-b", "潮音长枪", 5, ItemKind::Weapon, false, false},
        {"standard-four-hero", "逐风医师", 4, ItemKind::Character, false, false},
        {"standard-four-weapon", "铁影大剑", 4, ItemKind::Weapon, false, false},
        {"weapon-four-a", "鸣砂短剑", 4, ItemKind::Weapon, false, false},
        {"three-sword", "训练长剑", 3, ItemKind::Weapon, false, false},
        {"three-bow", "旧式猎弓", 3, ItemKind::Weapon, false, false},
        {"three-catalyst", "学徒法器", 3, ItemKind::Weapon, false, false},
    };

    return {
        {character.id, character},
        {weapon.id, weapon},
        {standard.id, standard},
    };
}

}

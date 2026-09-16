#include "gacha/WishEngine.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace gacha;

static void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static BannerConfig makeCharacterBanner() {
    BannerConfig banner;
    banner.id = "character-event";
    banner.name = "星辉同行";
    banner.type = BannerType::CharacterEvent;
    banner.fiveStarBaseRate = 0.006;
    banner.fiveStarHardPity = 90;
    banner.fourStarBaseRate = 0.051;
    banner.fourStarHardPity = 10;
    banner.items = {
        {"featured-hero", "星辉旅人", 5, ItemKind::Character, true, false},
        {"standard-hero", "晴岚术士", 5, ItemKind::Character, false, false},
        {"featured-four", "巡夜弓手", 4, ItemKind::Character, true, false},
        {"standard-four", "铁影大剑", 4, ItemKind::Weapon, false, false},
        {"three-star", "训练长剑", 3, ItemKind::Weapon, false, false},
    };
    return banner;
}

static BannerConfig makeWeaponBanner() {
    BannerConfig banner;
    banner.id = "weapon-event";
    banner.name = "辉铸兵装";
    banner.type = BannerType::WeaponEvent;
    banner.fiveStarBaseRate = 0.007;
    banner.fiveStarHardPity = 80;
    banner.fourStarBaseRate = 0.06;
    banner.fourStarHardPity = 10;
    banner.items = {
        {"weapon-a", "苍曜长弓", 5, ItemKind::Weapon, false, true},
        {"weapon-b", "赤砂法杖", 5, ItemKind::Weapon, false, true},
        {"weapon-standard", "白银阔刃", 5, ItemKind::Weapon, false, false},
        {"weapon-four", "鸣砂短剑", 4, ItemKind::Weapon, false, false},
        {"three-star", "训练长剑", 3, ItemKind::Weapon, false, false},
    };
    return banner;
}

static BannerConfig makeStandardBanner() {
    BannerConfig banner = makeCharacterBanner();
    banner.id = "standard";
    banner.name = "常驻星轨";
    banner.type = BannerType::Standard;
    banner.items[0].featured = false;
    return banner;
}

static void character_hard_pity_forces_five_star_at_90() {
    WishEngine engine;
    WishState state;
    SequenceRandom random(std::vector<double>(300, 0.99));
    auto banner = makeCharacterBanner();

    auto batch = engine.wish(banner, state, 90, random);

    require(batch.results.size() == 90, "expected 90 results");
    require(batch.results.back().item.rarity == 5, "90th wish must be 5-star");
    require(state.pity5 == 0, "5-star pity resets after hard pity hit");
}

static void standard_hard_pity_forces_five_star_at_90() {
    WishEngine engine;
    WishState state;
    SequenceRandom random(std::vector<double>(300, 0.99));
    auto banner = makeStandardBanner();

    auto batch = engine.wish(banner, state, 90, random);

    require(batch.results.back().item.rarity == 5, "standard 90th wish must be 5-star");
    require(state.pity5 == 0, "standard 5-star pity resets");
}

static void character_lost_fifty_fifty_guarantees_next_featured() {
    WishEngine engine;
    WishState state;
    auto banner = makeCharacterBanner();
    SequenceRandom random({0.0, 0.8});

    auto first = engine.wish(banner, state, 1, random);
    require(first.results[0].item.id == "standard-hero", "first 5-star should lose 50/50");
    require(state.featuredGuarantee, "losing 50/50 should set featured guarantee");

    SequenceRandom secondRandom({0.0});
    auto second = engine.wish(banner, state, 1, secondRandom);
    require(second.results[0].item.id == "featured-hero", "next 5-star should be featured");
    require(!state.featuredGuarantee, "featured guarantee should clear after use");
}

static void every_banner_four_star_hard_pity_at_10() {
    WishEngine engine;
    for (const auto& banner : {makeCharacterBanner(), makeWeaponBanner(), makeStandardBanner()}) {
        WishState state;
        SequenceRandom random(std::vector<double>(40, 0.99));
        auto batch = engine.wish(banner, state, 10, random);
        require(batch.results.back().item.rarity >= 4, banner.id + " 10th wish must be 4-star or above");
        require(state.pity4 == 0, banner.id + " 4-star pity resets");
    }
}

static void weapon_hard_pity_forces_five_star_at_80() {
    WishEngine engine;
    WishState state;
    SequenceRandom random(std::vector<double>(260, 0.99));
    auto banner = makeWeaponBanner();

    auto batch = engine.wish(banner, state, 80, random);

    require(batch.results.back().item.rarity == 5, "weapon 80th wish must be 5-star");
    require(state.pity5 == 0, "weapon 5-star pity resets");
}

static void weapon_lost_promotional_roll_guarantees_next_promotional() {
    WishEngine engine;
    WishState state;
    auto banner = makeWeaponBanner();
    SequenceRandom random({0.0, 0.9});

    auto first = engine.wish(banner, state, 1, random);
    require(first.results[0].item.id == "weapon-standard", "first weapon 5-star should be standard");
    require(state.promotionalGuarantee, "standard weapon 5-star should set promotional guarantee");

    SequenceRandom secondRandom({0.0, 0.2});
    auto second = engine.wish(banner, state, 1, secondRandom);
    require(second.results[0].item.promotional, "next weapon 5-star should be promotional");
    require(!state.promotionalGuarantee, "promotional guarantee should clear after use");
}

static void weapon_fate_points_force_selected_weapon() {
    WishEngine engine;
    WishState state;
    state.selectedPathItemId = "weapon-a";
    auto banner = makeWeaponBanner();

    SequenceRandom firstRandom({0.0, 0.9});
    auto first = engine.wish(banner, state, 1, firstRandom);
    require(first.results[0].item.id == "weapon-standard", "standard 5-star should miss selected path");
    require(state.fatePoints == 1, "missed selected weapon should add fate point");

    SequenceRandom secondRandom({0.0, 0.8});
    auto second = engine.wish(banner, state, 1, secondRandom);
    require(second.results[0].item.id == "weapon-b", "second promotional 5-star should miss selected path");
    require(state.fatePoints == 2, "second miss should bring fate points to 2");

    SequenceRandom thirdRandom({0.0});
    auto third = engine.wish(banner, state, 1, thirdRandom);
    require(third.results[0].item.id == "weapon-a", "fate points at 2 should force selected weapon");
    require(state.fatePoints == 0, "selected weapon should reset fate points");
}

int main() {
    try {
        character_hard_pity_forces_five_star_at_90();
        standard_hard_pity_forces_five_star_at_90();
        character_lost_fifty_fifty_guarantees_next_featured();
        every_banner_four_star_hard_pity_at_10();
        weapon_hard_pity_forces_five_star_at_80();
        weapon_lost_promotional_roll_guarantees_next_promotional();
        weapon_fate_points_force_selected_weapon();
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "WishEngineTests passed\n";
    return EXIT_SUCCESS;
}

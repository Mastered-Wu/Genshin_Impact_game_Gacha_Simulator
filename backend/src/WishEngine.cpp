#include "gacha/WishEngine.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace gacha {
namespace {

std::vector<Item> filterItems(const BannerConfig& banner, int rarity) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == rarity) {
            items.push_back(item);
        }
    }
    return items;
}

std::vector<Item> filterItems(const BannerConfig& banner, int rarity, bool featured, bool promotional) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == rarity && item.featured == featured && item.promotional == promotional) {
            items.push_back(item);
        }
    }
    return items;
}

std::vector<Item> promotionalFiveStars(const BannerConfig& banner) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == 5 && item.promotional) {
            items.push_back(item);
        }
    }
    return items;
}

Item chooseFrom(const std::vector<Item>& items, RandomProvider& random) {
    if (items.empty()) {
        throw std::runtime_error("banner item pool is empty for requested rarity");
    }
    if (items.size() == 1) {
        return items.front();
    }
    auto index = static_cast<std::size_t>(random.nextDouble() * items.size());
    if (index >= items.size()) {
        index = items.size() - 1;
    }
    return items[index];
}

Item chooseById(const BannerConfig& banner, const std::string& id) {
    auto found = std::find_if(banner.items.begin(), banner.items.end(), [&](const Item& item) {
        return item.id == id;
    });
    if (found == banner.items.end()) {
        throw std::runtime_error("selected path item is not in banner");
    }
    return *found;
}

Item chooseFiveStar(const BannerConfig& banner, WishState& state, RandomProvider& random, bool& usedGuarantee) {
    usedGuarantee = false;

    if (banner.type == BannerType::CharacterEvent) {
        auto featured = filterItems(banner, 5, true, false);
        auto standard = filterItems(banner, 5, false, false);
        if (state.featuredGuarantee) {
            usedGuarantee = true;
            state.featuredGuarantee = false;
            return chooseFrom(featured, random);
        }
        if (random.nextDouble() < 0.5) {
            state.featuredGuarantee = false;
            return chooseFrom(featured, random);
        }
        state.featuredGuarantee = true;
        return chooseFrom(standard, random);
    }

    if (banner.type == BannerType::WeaponEvent) {
        auto promotional = promotionalFiveStars(banner);
        auto standard = filterItems(banner, 5, false, false);
        Item selected;
        bool hasSelected = !state.selectedPathItemId.empty();

        if (hasSelected && state.fatePoints >= 2) {
            usedGuarantee = true;
            selected = chooseById(banner, state.selectedPathItemId);
        } else if (state.promotionalGuarantee) {
            usedGuarantee = true;
            selected = chooseFrom(promotional, random);
        } else if (random.nextDouble() < 0.75) {
            selected = chooseFrom(promotional, random);
        } else {
            selected = chooseFrom(standard, random);
        }

        if (selected.promotional) {
            state.promotionalGuarantee = false;
        } else {
            state.promotionalGuarantee = true;
        }

        if (hasSelected) {
            if (selected.id == state.selectedPathItemId) {
                state.fatePoints = 0;
            } else {
                state.fatePoints += 1;
            }
        }

        return selected;
    }

    return chooseFrom(filterItems(banner, 5), random);
}

void updateStats(WishState& state, const WishResult& result) {
    state.stats.total += 1;
    if (result.item.rarity == 5) {
        state.stats.fiveStars += 1;
        if (result.item.featured || result.item.promotional) {
            state.stats.featuredFiveStars += 1;
        }
    } else if (result.item.rarity == 4) {
        state.stats.fourStars += 1;
    }
}

}

WishBatch WishEngine::wish(const BannerConfig& banner, WishState& state, int count, RandomProvider& random) const {
    if (count <= 0) {
        throw std::invalid_argument("wish count must be positive");
    }

    WishBatch batch;
    for (int i = 0; i < count; ++i) {
        state.pity5 += 1;
        state.pity4 += 1;

        WishResult result;
        result.wishNumber = state.stats.total + 1;
        result.hitHardPity5 = state.pity5 >= banner.fiveStarHardPity;

        bool isFiveStar = result.hitHardPity5 || random.nextDouble() < banner.fiveStarBaseRate;
        if (isFiveStar) {
            bool usedGuarantee = false;
            result.item = chooseFiveStar(banner, state, random, usedGuarantee);
            result.usedGuarantee = usedGuarantee;
            state.pity5 = 0;
            state.pity4 = 0;
        } else {
            result.hitHardPity4 = state.pity4 >= banner.fourStarHardPity;
            bool isFourStar = result.hitHardPity4 || random.nextDouble() < banner.fourStarBaseRate;
            if (isFourStar) {
                result.item = chooseFrom(filterItems(banner, 4), random);
                state.pity4 = 0;
            } else {
                result.item = chooseFrom(filterItems(banner, 3), random);
            }
        }

        result.fatePointsAfter = state.fatePoints;
        updateStats(state, result);
        state.history.insert(state.history.begin(), result);
        batch.results.push_back(result);
    }

    batch.state = state;
    return batch;
}

}

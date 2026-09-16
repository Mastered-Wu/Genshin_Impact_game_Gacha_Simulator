#include "gacha/WishEngine.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace gacha {
namespace {

// 按稀有度筛选物品池，供 5/4/3 星结果选择使用。
std::vector<Item> filterItems(const BannerConfig& banner, int rarity) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == rarity) {
            items.push_back(item);
        }
    }
    return items;
}

// 按稀有度和限定标记筛选。角色活动池用 featured，武器活动池的普通 5 星也会走这里。
std::vector<Item> filterItems(const BannerConfig& banner, int rarity, bool featured, bool promotional) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == rarity && item.featured == featured && item.promotional == promotional) {
            items.push_back(item);
        }
    }
    return items;
}

// 武器活动池的两把限定 5 星使用 promotional 标记，和角色活动的 featured 分开。
std::vector<Item> promotionalFiveStars(const BannerConfig& banner) {
    std::vector<Item> items;
    for (const auto& item : banner.items) {
        if (item.rarity == 5 && item.promotional) {
            items.push_back(item);
        }
    }
    return items;
}

// 从候选物品中随机挑一个。随机值乘以候选数量得到下标，边界值做一次保护。
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

// 根据定轨 id 找到指定武器。走到这里说明前置 API 已经校验过 id，异常只防御坏状态。
Item chooseById(const BannerConfig& banner, const std::string& id) {
    auto found = std::find_if(banner.items.begin(), banner.items.end(), [&](const Item& item) {
        return item.id == id;
    });
    if (found == banner.items.end()) {
        throw std::runtime_error("selected path item is not in banner");
    }
    return *found;
}

// 计算本抽的 5 星实际概率。角色池配置为 73 抽后进入软保底：
// 第 74 抽起，每抽在基础概率上额外增加 fiveStarSoftPityIncrease。
double effectiveFiveStarRate(const BannerConfig& banner, int pity5AfterIncrement) {
    double rate = banner.fiveStarBaseRate;
    if (banner.fiveStarSoftPityStart > 0 && pity5AfterIncrement > banner.fiveStarSoftPityStart) {
        rate += (pity5AfterIncrement - banner.fiveStarSoftPityStart) * banner.fiveStarSoftPityIncrease;
    }
    return std::min(rate, 1.0);
}

// 处理“已经确定本抽是 5 星之后”的具体归属。
// 这里集中实现角色 50/50、角色限定保底、武器 75/25、武器限定保底和命定值强制。
Item chooseFiveStar(const BannerConfig& banner, WishState& state, RandomProvider& random, bool& usedGuarantee) {
    usedGuarantee = false;

    if (banner.type == BannerType::CharacterEvent) {
        auto featured = filterItems(banner, 5, true, false);
        auto standard = filterItems(banner, 5, false, false);

        // 角色池如果上一个 5 星歪了，本次 5 星直接给限定并清除保底标记。
        if (state.featuredGuarantee) {
            usedGuarantee = true;
            state.featuredGuarantee = false;
            return chooseFrom(featured, random);
        }

        // 无保底时走 50/50：赢则限定，输则常驻并设置下次限定保底。
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

        // 武器定轨优先级最高：命定值达到 2 后，下一个 5 星强制给所选武器。
        if (hasSelected && state.fatePoints >= 2) {
            usedGuarantee = true;
            selected = chooseById(banner, state.selectedPathItemId);
        } else if (state.promotionalGuarantee) {
            // 上一个 5 星如果是非限定，本次 5 星必定进入两把限定武器池。
            usedGuarantee = true;
            selected = chooseFrom(promotional, random);
        } else if (random.nextDouble() < 0.75) {
            // 无限定保底时，75% 概率进入两把限定武器池，25% 概率进入常驻 5 星武器池。
            selected = chooseFrom(promotional, random);
        } else {
            selected = chooseFrom(standard, random);
        }

        // 抽到非限定 5 星后设置“下个 5 星必限定”；抽到限定后清除这个标记。
        if (selected.promotional) {
            state.promotionalGuarantee = false;
        } else {
            state.promotionalGuarantee = true;
        }

        // 如果有定轨，任何非所选 5 星都会增加命定值；获得所选武器后命定值清零。
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
        // 每抽先推进保底计数。命中 5 星会同时清空 5 星和 4 星计数。
        state.pity5 += 1;
        state.pity4 += 1;

        WishResult result;
        result.wishNumber = state.stats.total + 1;
        result.hitHardPity5 = state.pity5 >= banner.fiveStarHardPity;

        // 5 星判定优先级最高：到硬保底必出，否则按当前抽数的有效概率判定。
        // 对角色活动池，第 73 抽后会逐抽提高概率；没有配置软保底的池仍使用基础概率。
        bool isFiveStar = result.hitHardPity5 || random.nextDouble() < effectiveFiveStarRate(banner, state.pity5);
        if (isFiveStar) {
            bool usedGuarantee = false;
            result.item = chooseFiveStar(banner, state, random, usedGuarantee);
            result.usedGuarantee = usedGuarantee;
            state.pity5 = 0;
            state.pity4 = 0;
        } else {
            result.hitHardPity4 = state.pity4 >= banner.fourStarHardPity;
            // 未命中 5 星时才判定 4 星。第 10 抽硬保底保证至少 4 星。
            bool isFourStar = result.hitHardPity4 || random.nextDouble() < banner.fourStarBaseRate;
            if (isFourStar) {
                result.item = chooseFrom(filterItems(banner, 4), random);
                state.pity4 = 0;
            } else {
                result.item = chooseFrom(filterItems(banner, 3), random);
            }
        }

        // 每次抽完把结果写入统计和历史。history 最新在前，方便前端直接渲染。
        result.fatePointsAfter = state.fatePoints;
        updateStats(state, result);
        state.history.insert(state.history.begin(), result);
        batch.results.push_back(result);
    }

    batch.state = state;
    return batch;
}

}

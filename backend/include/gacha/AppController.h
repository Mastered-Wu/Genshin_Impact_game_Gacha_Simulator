#pragma once

#include "gacha/BannerRepository.h"
#include "gacha/RandomProvider.h"
#include "gacha/WishEngine.h"

#include <map>
#include <string>

namespace gacha {

// AppController 是 HTTP 层和规则引擎之间的应用状态门面。
// 它持有所有卡池配置、每个卡池的独立状态，并把操作结果序列化成 JSON。
class AppController {
public:
    AppController();

    // 返回完整页面状态：卡池列表、所有卡池状态和默认当前卡池。
    std::string stateJson() const;

    // 执行单抽或十连。优先消耗缘券；allowCurrencyTopUp 为 true 时用星石补足缺口。
    std::string wishJson(const std::string& bannerId, int count, bool allowCurrencyTopUp = false);

    // 设置玩家当前资源数量。抽卡时按 160 资源一抽扣减。
    std::string setCurrencyJson(int currency);

    // 将星石兑换为当前卡池对应缘券。每 160 星石兑换 1 个缘券。
    std::string exchangeFatesJson(const std::string& bannerId, int fates);

    // 设置武器定轨。只有武器活动池允许设置，变更或清空定轨时重置命定值。
    std::string setPathJson(const std::string& bannerId, const std::string& itemId);

    // 重置所有卡池的运行状态，配置不变。
    std::string resetJson();

private:
    const BannerConfig* findBanner(const std::string& bannerId) const;
    WishState* findState(const std::string& bannerId);
    bool isValidPathItem(const BannerConfig& banner, const std::string& itemId) const;
    std::string responseWithState(const std::string& prefix) const;
    int fateBalanceFor(const BannerConfig& banner) const;
    int& mutableFateBalanceFor(const BannerConfig& banner);
    std::string fateNameFor(const BannerConfig& banner) const;

    std::map<std::string, BannerConfig> banners_;
    std::map<std::string, WishState> states_;
    int currency_ = 16000;
    int eventFates_ = 0;
    int standardFates_ = 0;
    int wishCost_ = 160;
    DefaultRandom random_;
    WishEngine engine_;
};

}

#pragma once

#include "gacha/BannerRepository.h"
#include "gacha/RandomProvider.h"
#include "gacha/WishEngine.h"

#include <map>
#include <string>

namespace gacha {

class AppController {
public:
    AppController();

    std::string stateJson() const;
    std::string wishJson(const std::string& bannerId, int count);
    std::string setPathJson(const std::string& bannerId, const std::string& itemId);
    std::string resetJson();

private:
    const BannerConfig* findBanner(const std::string& bannerId) const;
    WishState* findState(const std::string& bannerId);
    bool isValidPathItem(const BannerConfig& banner, const std::string& itemId) const;
    std::string responseWithState(const std::string& prefix) const;

    std::map<std::string, BannerConfig> banners_;
    std::map<std::string, WishState> states_;
    DefaultRandom random_;
    WishEngine engine_;
};

}

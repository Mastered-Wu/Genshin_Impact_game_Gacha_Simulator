#pragma once

#include "gacha/Models.h"

#include <map>
#include <string>

namespace gacha {

std::string escapeJson(const std::string& value);
std::string itemJson(const Item& item);
std::string bannerJson(const BannerConfig& banner);
std::string resultJson(const WishResult& result);
std::string stateJson(const WishState& state);
std::string allStatesJson(const std::map<std::string, WishState>& states);
std::string errorJson(const std::string& code, const std::string& message);

}

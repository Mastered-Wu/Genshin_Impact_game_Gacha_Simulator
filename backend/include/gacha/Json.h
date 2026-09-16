#pragma once

#include "gacha/Models.h"

#include <map>
#include <string>

namespace gacha {

// 下面这些函数只负责把后端模型拼成 API 响应 JSON。
// 当前项目字段固定且结构简单，所以用手写序列化；所有字符串都先经过 escapeJson。
std::string escapeJson(const std::string& value);
std::string itemJson(const Item& item);
std::string bannerJson(const BannerConfig& banner);
std::string resultJson(const WishResult& result);
std::string stateJson(const WishState& state);
std::string allStatesJson(const std::map<std::string, WishState>& states);
std::string errorJson(const std::string& code, const std::string& message);

}

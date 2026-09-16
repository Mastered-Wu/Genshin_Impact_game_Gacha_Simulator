#pragma once

#include "gacha/Models.h"

#include <map>
#include <string>

namespace gacha {

std::map<std::string, BannerConfig> loadBuiltInBanners();

}

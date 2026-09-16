#pragma once

#include "gacha/Models.h"
#include "gacha/RandomProvider.h"

namespace gacha {

class WishEngine {
public:
    WishBatch wish(const BannerConfig& banner, WishState& state, int count, RandomProvider& random) const;
};

}

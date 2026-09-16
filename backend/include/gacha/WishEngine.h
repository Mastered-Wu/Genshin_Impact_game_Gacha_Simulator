#pragma once

#include "gacha/Models.h"
#include "gacha/RandomProvider.h"

namespace gacha {

class WishEngine {
public:
    // 执行 count 次抽卡并原地更新 state。
    // 规则流程在实现中集中处理：先判定 5 星硬保底/概率，再判定 4 星保底/概率，
    // 命中 5 星后按卡池类型应用限定保底或武器命定值。
    WishBatch wish(const BannerConfig& banner, WishState& state, int count, RandomProvider& random) const;
};

}

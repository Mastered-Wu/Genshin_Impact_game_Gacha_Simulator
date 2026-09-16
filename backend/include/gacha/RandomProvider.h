#pragma once

#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gacha {

// 随机数抽象。规则引擎只依赖这个接口，因此测试可以注入固定序列。
class RandomProvider {
public:
    virtual ~RandomProvider() = default;

    // 返回 [0, 1) 区间的小数，用于判定概率和从物品池中随机挑选。
    virtual double nextDouble() = 0;
};

// 生产环境随机源，使用标准库随机设备初始化 mt19937。
class DefaultRandom final : public RandomProvider {
public:
    double nextDouble() override {
        return distribution_(engine_);
    }

private:
    std::mt19937 engine_{std::random_device{}()};
    std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

// 测试随机源。按顺序吐出预置数字，让硬保底、歪/不歪等场景可重复验证。
class SequenceRandom final : public RandomProvider {
public:
    explicit SequenceRandom(std::vector<double> values) : values_(std::move(values)) {}

    double nextDouble() override {
        if (index_ >= values_.size()) {
            throw std::runtime_error("SequenceRandom exhausted");
        }
        return values_[index_++];
    }

private:
    std::vector<double> values_;
    std::size_t index_ = 0;
};

}

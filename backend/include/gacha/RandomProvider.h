#pragma once

#include <cstddef>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace gacha {

class RandomProvider {
public:
    virtual ~RandomProvider() = default;
    virtual double nextDouble() = 0;
};

class DefaultRandom final : public RandomProvider {
public:
    double nextDouble() override {
        return distribution_(engine_);
    }

private:
    std::mt19937 engine_{std::random_device{}()};
    std::uniform_real_distribution<double> distribution_{0.0, 1.0};
};

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

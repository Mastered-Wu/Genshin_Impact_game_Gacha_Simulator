#include "gacha/AppController.h"

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace gacha;

static void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

static void default_state_contains_three_banners() {
    AppController app;
    auto json = app.stateJson();
    require(json.find("\"character-event\"") != std::string::npos, "state should include character banner");
    require(json.find("\"weapon-event\"") != std::string::npos, "state should include weapon banner");
    require(json.find("\"standard\"") != std::string::npos, "state should include standard banner");
}

static void invalid_wish_count_returns_stable_code() {
    AppController app;
    auto json = app.wishJson("character-event", 3);
    require(json.find("\"code\":\"invalid_count\"") != std::string::npos, "invalid count should return invalid_count");
}

static void resources_gate_wishes_and_deduct_cost() {
    AppController app;

    auto insufficient = app.wishJson("character-event", 1);
    require(insufficient.find("\"code\":\"insufficient_currency\"") != std::string::npos, "wish without resources should fail");

    auto invalid = app.setCurrencyJson(-1);
    require(invalid.find("\"code\":\"invalid_currency\"") != std::string::npos, "negative resources should be invalid");

    auto funded = app.setCurrencyJson(1600);
    require(funded.find("\"currency\":1600") != std::string::npos, "resources should be set");

    auto wished = app.wishJson("character-event", 10);
    require(wished.find("\"results\"") != std::string::npos, "funded ten-pull should return results");
    require(wished.find("\"currency\":0") != std::string::npos, "ten-pull should deduct 1600 resources");
}

int main() {
    try {
        default_state_contains_three_banners();
        invalid_wish_count_returns_stable_code();
        resources_gate_wishes_and_deduct_cost();
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "ApiSmokeTests passed\n";
    return EXIT_SUCCESS;
}

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

int main() {
    try {
        default_state_contains_three_banners();
        invalid_wish_count_returns_stable_code();
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "ApiSmokeTests passed\n";
    return EXIT_SUCCESS;
}

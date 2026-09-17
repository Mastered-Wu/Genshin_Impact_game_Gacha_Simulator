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

    app.setCurrencyJson(0);
    auto insufficient = app.wishJson("character-event", 1);
    require(insufficient.find("\"code\":\"need_currency_confirm\"") != std::string::npos, "wish without fates should ask for confirmation first");
    auto confirmedInsufficient = app.wishJson("character-event", 1, true);
    require(confirmedInsufficient.find("\"code\":\"insufficient_currency\"") != std::string::npos, "confirmed wish without resources should fail");

    auto invalid = app.setCurrencyJson(-1);
    require(invalid.find("\"code\":\"invalid_currency\"") != std::string::npos, "negative resources should be invalid");

    auto funded = app.setCurrencyJson(1600);
    require(funded.find("\"currency\":1600") != std::string::npos, "resources should be set");

    auto wished = app.wishJson("character-event", 10, true);
    require(wished.find("\"results\"") != std::string::npos, "funded ten-pull should return results");
    require(wished.find("\"currency\":0") != std::string::npos, "ten-pull should deduct 1600 resources");
}

static void exchange_adds_fates_and_wishes_use_fates_first() {
    AppController app;

    auto exchanged = app.exchangeFatesJson("character-event", 2);
    require(exchanged.find("\"eventFates\":2") != std::string::npos, "limited exchange should add event fates");
    require(exchanged.find("\"standardFates\":0") != std::string::npos, "limited exchange should not add standard fates");
    require(exchanged.find("\"currency\":15680") != std::string::npos, "exchange should deduct currency");

    auto wished = app.wishJson("character-event", 1);
    require(wished.find("\"results\"") != std::string::npos, "wish should consume fate without confirmation");
    require(wished.find("\"eventFates\":1") != std::string::npos, "limited wish should use event fates first");
    require(wished.find("\"currency\":15680") != std::string::npos, "wish with fate should not spend currency");

    auto invalid = app.exchangeFatesJson("character-event", -1);
    require(invalid.find("\"code\":\"invalid_exchange_count\"") != std::string::npos, "negative exchange should be invalid");
}

static void standard_banner_uses_separate_fates() {
    AppController app;

    auto exchanged = app.exchangeFatesJson("standard", 1);
    require(exchanged.find("\"standardFates\":1") != std::string::npos, "standard exchange should add standard fates");
    require(exchanged.find("\"eventFates\":0") != std::string::npos, "standard exchange should not add event fates");
    require(exchanged.find("恒辉之缘") != std::string::npos, "standard exchange should report standard fate name");

    auto eventWish = app.wishJson("character-event", 1);
    require(eventWish.find("\"code\":\"need_currency_confirm\"") != std::string::npos, "event wish should not use standard fates");
    require(eventWish.find("星轨之缘") != std::string::npos, "event wish should report event fate name");

    auto standardWish = app.wishJson("standard", 1);
    require(standardWish.find("\"results\"") != std::string::npos, "standard wish should consume standard fate");
    require(standardWish.find("\"standardFates\":0") != std::string::npos, "standard wish should deduct standard fate");
}

int main() {
    try {
        default_state_contains_three_banners();
        invalid_wish_count_returns_stable_code();
        resources_gate_wishes_and_deduct_cost();
        exchange_adds_fates_and_wishes_use_fates_first();
        standard_banner_uses_separate_fates();
    } catch (const std::exception& ex) {
        std::cerr << "FAIL: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }
    std::cout << "ApiSmokeTests passed\n";
    return EXIT_SUCCESS;
}

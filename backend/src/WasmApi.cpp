#include "gacha/AppController.h"

#include <emscripten.h>

#include <string>
#include <utility>

EM_JS(double, browserRandomValue, (), {
    const value = new Uint32Array(1);
    globalThis.crypto.getRandomValues(value);
    return value[0] / 4294967296;
});

namespace {

class BrowserRandomProvider final : public gacha::RandomProvider {
public:
    double nextDouble() override {
        return browserRandomValue();
    }
};

gacha::AppController& app() {
    static BrowserRandomProvider random;
    static gacha::AppController controller(random);
    return controller;
}

const char* retainResponse(std::string response) {
    static std::string result;
    result = std::move(response);
    return result.c_str();
}

}

extern "C" {

EMSCRIPTEN_KEEPALIVE const char* gacha_state() {
    return retainResponse(app().stateJson());
}

EMSCRIPTEN_KEEPALIVE const char* gacha_wish(const char* bannerId, int count, int allowCurrencyTopUp) {
    return retainResponse(app().wishJson(
        bannerId == nullptr ? "" : bannerId,
        count,
        allowCurrencyTopUp != 0));
}

EMSCRIPTEN_KEEPALIVE const char* gacha_resources(int currency) {
    return retainResponse(app().setCurrencyJson(currency));
}

EMSCRIPTEN_KEEPALIVE const char* gacha_exchange(const char* bannerId, int fates) {
    return retainResponse(app().exchangeFatesJson(bannerId == nullptr ? "" : bannerId, fates));
}

EMSCRIPTEN_KEEPALIVE const char* gacha_path(const char* bannerId, const char* itemId) {
    return retainResponse(app().setPathJson(
        bannerId == nullptr ? "" : bannerId,
        itemId == nullptr ? "" : itemId));
}

EMSCRIPTEN_KEEPALIVE const char* gacha_reset() {
    return retainResponse(app().resetJson());
}

}

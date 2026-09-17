#include "gacha/AppController.h"
#include "gacha/Json.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

namespace gacha {
namespace {

// 把所有卡池配置组装成 JSON 数组，供 /api/state 和操作响应复用。
std::string bannersArrayJson(const std::map<std::string, BannerConfig>& banners) {
    std::ostringstream out;
    out << "[";
    std::size_t index = 0;
    for (const auto& entry : banners) {
        if (index++ > 0) {
            out << ",";
        }
        out << bannerJson(entry.second);
    }
    out << "]";
    return out.str();
}

// 从简单 JSON 请求体中取字符串字段。
// 当前 API 只接收很小的固定结构，所以这里不用引入额外 JSON 库。
std::string jsonStringField(const std::string& body, const std::string& field) {
    const std::string needle = "\"" + field + "\"";
    auto pos = body.find(needle);
    if (pos == std::string::npos) {
        return "";
    }
    pos = body.find(':', pos);
    if (pos == std::string::npos) {
        return "";
    }
    pos = body.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos || body.compare(pos, 4, "null") == 0) {
        return "";
    }
    if (body[pos] != '"') {
        return "";
    }
    ++pos;
    std::string value;
    bool escaped = false;
    for (; pos < body.size(); ++pos) {
        char ch = body[pos];
        if (escaped) {
            value.push_back(ch);
            escaped = false;
        } else if (ch == '\\') {
            escaped = true;
        } else if (ch == '"') {
            break;
        } else {
            value.push_back(ch);
        }
    }
    return value;
}

// 从简单 JSON 请求体中取整数字段，例如 /api/wish 的 count。
std::optional<int> jsonIntField(const std::string& body, const std::string& field) {
    const std::string needle = "\"" + field + "\"";
    auto pos = body.find(needle);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = body.find(':', pos);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    pos = body.find_first_not_of(" \t\r\n", pos + 1);
    if (pos == std::string::npos) {
        return std::nullopt;
    }
    int value = 0;
    bool foundDigit = false;
    for (; pos < body.size() && body[pos] >= '0' && body[pos] <= '9'; ++pos) {
        foundDigit = true;
        value = value * 10 + (body[pos] - '0');
    }
    if (!foundDigit) {
        return std::nullopt;
    }
    return value;
}

bool jsonBoolField(const std::string& body, const std::string& field) {
    const std::string needle = "\"" + field + "\"";
    auto pos = body.find(needle);
    if (pos == std::string::npos) {
        return false;
    }
    pos = body.find(':', pos);
    if (pos == std::string::npos) {
        return false;
    }
    pos = body.find_first_not_of(" \t\r\n", pos + 1);
    return pos != std::string::npos && body.compare(pos, 4, "true") == 0;
}

// 定位前端目录。支持从项目根目录运行，也支持从 build 目录附近运行。
std::filesystem::path frontendRoot() {
    const auto cwd = std::filesystem::current_path();
    const auto fromRoot = cwd / "frontend";
    if (std::filesystem::exists(fromRoot / "index.html")) {
        return fromRoot;
    }
    const auto fromBuild = cwd.parent_path() / "frontend";
    if (std::filesystem::exists(fromBuild / "index.html")) {
        return fromBuild;
    }
    return fromRoot;
}

// 读取静态文件内容，用于返回 index.html、styles.css 和 app.js。
std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// 根据文件后缀返回浏览器需要的 Content-Type。
std::string contentTypeFor(const std::string& path) {
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=utf-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript; charset=utf-8";
    }
    return "text/html; charset=utf-8";
}

// 拼接最小 HTTP 响应头和响应体。
std::string httpResponse(int status, const std::string& statusText, const std::string& contentType, const std::string& body) {
    std::ostringstream out;
    out << "HTTP/1.1 " << status << " " << statusText << "\r\n"
        << "Content-Type: " << contentType << "\r\n"
        << "Content-Length: " << body.size() << "\r\n"
        << "Connection: close\r\n"
        << "Access-Control-Allow-Origin: *\r\n"
        << "\r\n"
        << body;
    return out.str();
}

}

AppController::AppController() : banners_(loadBuiltInBanners()) {
    // 每个卡池都有独立状态，角色、武器、常驻互不共享保底。
    for (const auto& entry : banners_) {
        states_.emplace(entry.first, WishState{});
    }
}

const BannerConfig* AppController::findBanner(const std::string& bannerId) const {
    auto found = banners_.find(bannerId);
    if (found == banners_.end()) {
        return nullptr;
    }
    return &found->second;
}

WishState* AppController::findState(const std::string& bannerId) {
    auto found = states_.find(bannerId);
    if (found == states_.end()) {
        return nullptr;
    }
    return &found->second;
}

bool AppController::isValidPathItem(const BannerConfig& banner, const std::string& itemId) const {
    return std::any_of(banner.items.begin(), banner.items.end(), [&](const Item& item) {
        return item.id == itemId && item.rarity == 5 && item.promotional;
    });
}

int AppController::fateBalanceFor(const BannerConfig& banner) const {
    return banner.type == BannerType::Standard ? standardFates_ : eventFates_;
}

int& AppController::mutableFateBalanceFor(const BannerConfig& banner) {
    return banner.type == BannerType::Standard ? standardFates_ : eventFates_;
}

std::string AppController::fateNameFor(const BannerConfig& banner) const {
    return banner.type == BannerType::Standard ? "恒辉之缘" : "星轨之缘";
}

std::string AppController::responseWithState(const std::string& prefix) const {
    // 操作接口会通过 prefix 塞入 results/bannerId/reset，再统一带上完整状态。
    std::ostringstream out;
    out << "{" << prefix
        << "\"banners\":" << bannersArrayJson(banners_)
        << ",\"states\":" << allStatesJson(states_)
        << ",\"resources\":{\"currency\":" << currency_
        << ",\"eventFates\":" << eventFates_
        << ",\"standardFates\":" << standardFates_
        << ",\"wishCost\":" << wishCost_
        << ",\"exchangeableFates\":" << currency_ / wishCost_
        << ",\"affordableEventWishes\":" << eventFates_ + (currency_ / wishCost_)
        << ",\"affordableStandardWishes\":" << standardFates_ + (currency_ / wishCost_)
        << "}"
        << ",\"currentBannerId\":\"character-event\""
        << "}";
    return out.str();
}

std::string AppController::stateJson() const {
    return responseWithState("");
}

std::string AppController::wishJson(const std::string& bannerId, int count, bool allowCurrencyTopUp) {
    const auto* banner = findBanner(bannerId);
    auto* state = findState(bannerId);
    if (banner == nullptr || state == nullptr) {
        return errorJson("unknown_banner", "未知卡池");
    }
    if (count != 1 && count != 10) {
        return errorJson("invalid_count", "抽卡次数只能是 1 或 10");
    }
    int& fateBalance = mutableFateBalanceFor(*banner);
    const int missingFates = std::max(0, count - fateBalance);
    const int requiredCurrency = missingFates * wishCost_;
    if (missingFates > 0 && !allowCurrencyTopUp) {
        std::ostringstream out;
        out << "{\"error\":\"缘券不足\",\"code\":\"need_currency_confirm\""
            << ",\"missingFates\":" << missingFates
            << ",\"requiredCurrency\":" << requiredCurrency
            << ",\"fateName\":\"" << escapeJson(fateNameFor(*banner)) << "\""
            << ",\"resources\":{\"currency\":" << currency_
            << ",\"eventFates\":" << eventFates_
            << ",\"standardFates\":" << standardFates_
            << ",\"wishCost\":" << wishCost_
            << ",\"exchangeableFates\":" << currency_ / wishCost_
            << ",\"affordableEventWishes\":" << eventFates_ + (currency_ / wishCost_)
            << ",\"affordableStandardWishes\":" << standardFates_ + (currency_ / wishCost_)
            << "}}";
        return out.str();
    }
    if (currency_ < requiredCurrency) {
        return errorJson("insufficient_currency", "资源不足，无法完成本次抽卡");
    }

    // AppController 只做参数校验和状态保存，抽卡规则全部委托给 WishEngine。
    auto batch = engine_.wish(*banner, *state, count, random_);
    const int fatesUsed = std::min(fateBalance, count);
    fateBalance -= fatesUsed;
    currency_ -= requiredCurrency;
    std::ostringstream results;
    results << "\"results\":[";
    for (std::size_t i = 0; i < batch.results.size(); ++i) {
        if (i > 0) {
            results << ",";
        }
        results << resultJson(batch.results[i]);
    }
    results << "],\"bannerId\":\"" << escapeJson(bannerId) << "\",";
    return responseWithState(results.str());
}

std::string AppController::setCurrencyJson(int currency) {
    if (currency < 0) {
        return errorJson("invalid_currency", "资源数量不能小于 0");
    }
    currency_ = currency;
    return responseWithState("\"resourcesUpdated\":true,");
}

std::string AppController::exchangeFatesJson(const std::string& bannerId, int fates) {
    const auto* banner = findBanner(bannerId);
    if (banner == nullptr) {
        return errorJson("unknown_banner", "未知卡池");
    }
    if (fates < 0) {
        return errorJson("invalid_exchange_count", "兑换数量不能小于 0");
    }
    const int totalCost = fates * wishCost_;
    if (currency_ < totalCost) {
        return errorJson("insufficient_currency", "星石不足，无法完成兑换");
    }
    currency_ -= totalCost;
    mutableFateBalanceFor(*banner) += fates;
    return responseWithState("\"resourcesUpdated\":true,\"exchangedFates\":" + std::to_string(fates)
        + ",\"fateName\":\"" + escapeJson(fateNameFor(*banner)) + "\",");
}

std::string AppController::setPathJson(const std::string& bannerId, const std::string& itemId) {
    const auto* banner = findBanner(bannerId);
    auto* state = findState(bannerId);
    if (banner == nullptr || state == nullptr) {
        return errorJson("unknown_banner", "未知卡池");
    }
    if (banner->type != BannerType::WeaponEvent) {
        return errorJson("invalid_path_item", "只有武器活动祈愿可以定轨");
    }
    if (!itemId.empty() && !isValidPathItem(*banner, itemId)) {
        return errorJson("invalid_path_item", "无效的定轨目标");
    }
    // 定轨目标变化或被清空时，命定值必须归零。
    if (state->selectedPathItemId != itemId) {
        state->selectedPathItemId = itemId;
        state->fatePoints = 0;
    }
    return responseWithState("\"bannerId\":\"" + escapeJson(bannerId) + "\",");
}

std::string AppController::resetJson() {
    states_.clear();
    for (const auto& entry : banners_) {
        states_.emplace(entry.first, WishState{});
    }
    currency_ = 16000;
    eventFates_ = 0;
    standardFates_ = 0;
    return responseWithState("\"reset\":true,");
}

}

#ifndef GACHA_TESTING
int main() {
#ifdef _WIN32
    // Windows 下使用 Winsock 创建一个仅监听 127.0.0.1 的本地 HTTP 服务。
    WSADATA data;
    if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
        std::cerr << "Failed to initialize Winsock\n";
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cerr << "Failed to create socket\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");
    address.sin_port = htons(18080);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Failed to bind 127.0.0.1:18080\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Failed to listen\n";
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    gacha::AppController app;
    std::cout << "Gacha simulator running at http://127.0.0.1:18080\n";

    while (true) {
        // 这个服务器是单线程、短连接模型：接收一个请求，生成响应，然后关闭连接。
        SOCKET client = accept(serverSocket, nullptr, nullptr);
        if (client == INVALID_SOCKET) {
            continue;
        }

        std::string request;
        char buffer[4096];
        int received = recv(client, buffer, sizeof(buffer), 0);
        if (received > 0) {
            request.assign(buffer, buffer + received);
        }

        auto firstLineEnd = request.find("\r\n");
        std::string firstLine = firstLineEnd == std::string::npos ? request : request.substr(0, firstLineEnd);
        std::istringstream firstLineStream(firstLine);
        std::string method;
        std::string path;
        firstLineStream >> method >> path;

        auto bodyStart = request.find("\r\n\r\n");
        std::string body = bodyStart == std::string::npos ? "" : request.substr(bodyStart + 4);
        std::string response;

        // API 路由返回 JSON；其它路径按静态文件处理。
        if (method == "GET" && path == "/api/state") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8", app.stateJson());
        } else if (method == "POST" && path == "/api/wish") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8",
                app.wishJson(gacha::jsonStringField(body, "bannerId"), gacha::jsonIntField(body, "count").value_or(0),
                    gacha::jsonBoolField(body, "allowCurrencyTopUp")));
        } else if (method == "POST" && path == "/api/resources") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8",
                app.setCurrencyJson(gacha::jsonIntField(body, "currency").value_or(-1)));
        } else if (method == "POST" && path == "/api/exchange") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8",
                app.exchangeFatesJson(gacha::jsonStringField(body, "bannerId"), gacha::jsonIntField(body, "fates").value_or(-1)));
        } else if (method == "POST" && path == "/api/path") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8",
                app.setPathJson(gacha::jsonStringField(body, "bannerId"), gacha::jsonStringField(body, "itemId")));
        } else if (method == "POST" && path == "/api/reset") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8", app.resetJson());
        } else {
            std::string filePath = path == "/" ? "/index.html" : path;
            if (filePath.find("..") != std::string::npos) {
                response = gacha::httpResponse(400, "Bad Request", "application/json; charset=utf-8",
                    gacha::errorJson("bad_path", "无效路径"));
            } else {
                auto content = gacha::readTextFile(gacha::frontendRoot() / filePath.substr(1));
                if (content.empty()) {
                    response = gacha::httpResponse(404, "Not Found", "application/json; charset=utf-8",
                        gacha::errorJson("not_found", "资源不存在"));
                } else {
                    response = gacha::httpResponse(200, "OK", gacha::contentTypeFor(filePath), content);
                }
            }
        }

        send(client, response.c_str(), static_cast<int>(response.size()), 0);
        closesocket(client);
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
#else
    std::cerr << "This minimal server currently supports Windows builds only.\n";
    return 1;
#endif
}
#endif

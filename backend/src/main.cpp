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

std::string readTextFile(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string contentTypeFor(const std::string& path) {
    if (path.size() >= 4 && path.substr(path.size() - 4) == ".css") {
        return "text/css; charset=utf-8";
    }
    if (path.size() >= 3 && path.substr(path.size() - 3) == ".js") {
        return "application/javascript; charset=utf-8";
    }
    return "text/html; charset=utf-8";
}

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

std::string AppController::responseWithState(const std::string& prefix) const {
    std::ostringstream out;
    out << "{" << prefix
        << "\"banners\":" << bannersArrayJson(banners_)
        << ",\"states\":" << allStatesJson(states_)
        << ",\"currentBannerId\":\"character-event\""
        << "}";
    return out.str();
}

std::string AppController::stateJson() const {
    return responseWithState("");
}

std::string AppController::wishJson(const std::string& bannerId, int count) {
    const auto* banner = findBanner(bannerId);
    auto* state = findState(bannerId);
    if (banner == nullptr || state == nullptr) {
        return errorJson("unknown_banner", "未知卡池");
    }
    if (count != 1 && count != 10) {
        return errorJson("invalid_count", "抽卡次数只能是 1 或 10");
    }

    auto batch = engine_.wish(*banner, *state, count, random_);
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
    return responseWithState("\"reset\":true,");
}

}

#ifndef GACHA_TESTING
int main() {
#ifdef _WIN32
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

        if (method == "GET" && path == "/api/state") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8", app.stateJson());
        } else if (method == "POST" && path == "/api/wish") {
            response = gacha::httpResponse(200, "OK", "application/json; charset=utf-8",
                app.wishJson(gacha::jsonStringField(body, "bannerId"), gacha::jsonIntField(body, "count").value_or(0)));
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

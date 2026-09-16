#include "gacha/Json.h"

#include <sstream>

namespace gacha {
namespace {

std::string bannerTypeJson(BannerType type) {
    switch (type) {
    case BannerType::CharacterEvent:
        return "character-event";
    case BannerType::WeaponEvent:
        return "weapon-event";
    case BannerType::Standard:
        return "standard";
    }
    return "standard";
}

std::string itemKindJson(ItemKind kind) {
    return kind == ItemKind::Character ? "character" : "weapon";
}

}

std::string escapeJson(const std::string& value) {
    std::ostringstream out;
    for (char ch : value) {
        switch (ch) {
        case '\\':
            out << "\\\\";
            break;
        case '"':
            out << "\\\"";
            break;
        case '\n':
            out << "\\n";
            break;
        case '\r':
            out << "\\r";
            break;
        case '\t':
            out << "\\t";
            break;
        default:
            out << ch;
            break;
        }
    }
    return out.str();
}

std::string itemJson(const Item& item) {
    std::ostringstream out;
    out << "{\"id\":\"" << escapeJson(item.id)
        << "\",\"name\":\"" << escapeJson(item.name)
        << "\",\"rarity\":" << item.rarity
        << ",\"kind\":\"" << itemKindJson(item.kind)
        << "\",\"featured\":" << (item.featured ? "true" : "false")
        << ",\"promotional\":" << (item.promotional ? "true" : "false")
        << "}";
    return out.str();
}

std::string bannerJson(const BannerConfig& banner) {
    std::ostringstream out;
    out << "{\"id\":\"" << escapeJson(banner.id)
        << "\",\"name\":\"" << escapeJson(banner.name)
        << "\",\"type\":\"" << bannerTypeJson(banner.type)
        << "\",\"fiveStarHardPity\":" << banner.fiveStarHardPity
        << ",\"fourStarHardPity\":" << banner.fourStarHardPity
        << ",\"items\":[";
    for (std::size_t i = 0; i < banner.items.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << itemJson(banner.items[i]);
    }
    out << "],\"pathItems\":[";
    bool first = true;
    for (const auto& item : banner.items) {
        if (item.rarity == 5 && item.promotional) {
            if (!first) {
                out << ",";
            }
            out << itemJson(item);
            first = false;
        }
    }
    out << "]}";
    return out.str();
}

std::string resultJson(const WishResult& result) {
    std::ostringstream out;
    out << "{\"wishNumber\":" << result.wishNumber
        << ",\"item\":" << itemJson(result.item)
        << ",\"hitHardPity5\":" << (result.hitHardPity5 ? "true" : "false")
        << ",\"hitHardPity4\":" << (result.hitHardPity4 ? "true" : "false")
        << ",\"usedGuarantee\":" << (result.usedGuarantee ? "true" : "false")
        << ",\"fatePointsAfter\":" << result.fatePointsAfter
        << "}";
    return out.str();
}

std::string stateJson(const WishState& state) {
    std::ostringstream out;
    out << "{\"pity5\":" << state.pity5
        << ",\"pity4\":" << state.pity4
        << ",\"featuredGuarantee\":" << (state.featuredGuarantee ? "true" : "false")
        << ",\"promotionalGuarantee\":" << (state.promotionalGuarantee ? "true" : "false")
        << ",\"fatePoints\":" << state.fatePoints
        << ",\"selectedPathItemId\":";
    if (state.selectedPathItemId.empty()) {
        out << "null";
    } else {
        out << "\"" << escapeJson(state.selectedPathItemId) << "\"";
    }
    out << ",\"stats\":{\"total\":" << state.stats.total
        << ",\"fiveStars\":" << state.stats.fiveStars
        << ",\"fourStars\":" << state.stats.fourStars
        << ",\"featuredFiveStars\":" << state.stats.featuredFiveStars
        << "},\"history\":[";
    for (std::size_t i = 0; i < state.history.size(); ++i) {
        if (i > 0) {
            out << ",";
        }
        out << resultJson(state.history[i]);
    }
    out << "]}";
    return out.str();
}

std::string allStatesJson(const std::map<std::string, WishState>& states) {
    std::ostringstream out;
    out << "{";
    std::size_t index = 0;
    for (const auto& entry : states) {
        if (index++ > 0) {
            out << ",";
        }
        out << "\"" << escapeJson(entry.first) << "\":" << stateJson(entry.second);
    }
    out << "}";
    return out.str();
}

std::string errorJson(const std::string& code, const std::string& message) {
    return "{\"error\":\"" + escapeJson(message) + "\",\"code\":\"" + escapeJson(code) + "\"}";
}

}

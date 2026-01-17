#include "category.h"
#include <cctype>

std::string naive_risk_classifier(const std::string& prompt) {
    std::string lower = prompt;
    for (char& c : lower) {
        c = std::tolower(c);
    }

    if (lower.find("ignore") != std::string::npos ||
        lower.find("disregard") != std::string::npos ||
        lower.find("forget") != std::string::npos ||
        lower.find("override") != std::string::npos) {
        return "LLM01";
    }

    if (lower.find("sql") != std::string::npos ||
        lower.find("delete") != std::string::npos ||
        lower.find("drop table") != std::string::npos ||
        lower.find("<script") != std::string::npos) {
        return "LLM02";
    }

    if (lower.find("system prompt") != std::string::npos ||
        lower.find("instructions") != std::string::npos ||
        lower.find("tell me your") != std::string::npos) {
        return "LLM06";
    }

    if (prompt.length() > 1000) {
        return "LLM04";
    }

    return "unknown";
}


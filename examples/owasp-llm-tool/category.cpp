#include "category.h"
#include <cctype>

std::string naive_risk_classifier(const std::string& prompt) {
    std::string lower = prompt;
    for (char& c : lower) {
        c = std::tolower(c);
    }

    // LLM04: DoS (check first - simple length)
    if (prompt.length() > 1000) {
        return "LLM04";
    }

    // LLM02: Insecure Output (specific injection patterns)
    if (lower.find("select * from") != std::string::npos ||
        lower.find("drop table") != std::string::npos ||
        lower.find("drop database") != std::string::npos ||
        lower.find("delete from") != std::string::npos ||
        lower.find("<script") != std::string::npos ||
        lower.find("';--") != std::string::npos) {
        return "LLM02";
    }

    // LLM06: Excessive Agency (system extraction - more specific)
    if (lower.find("system prompt") != std::string::npos ||
        lower.find("instructions") != std::string::npos ||
        lower.find("your guidelines") != std::string::npos ||
        lower.find("your configuration") != std::string::npos ||
        lower.find("your rules") != std::string::npos ||
        lower.find("reveal your") != std::string::npos ||
        lower.find("show me your") != std::string::npos ||
        lower.find("tell me your prompt") != std::string::npos) {
        return "LLM06";
    }

    // LLM01: Prompt Injection (broader override patterns)
    if (lower.find("ignore") != std::string::npos ||
        lower.find("disregard") != std::string::npos ||
        lower.find("forget") != std::string::npos ||
        lower.find("override") != std::string::npos ||
        lower.find("new instruction") != std::string::npos ||
        lower.find("system:") != std::string::npos ||
        lower.find("bypass") != std::string::npos ||
        lower.find("reset") != std::string::npos) {
        return "LLM01";
    }

    return "unknown";
}


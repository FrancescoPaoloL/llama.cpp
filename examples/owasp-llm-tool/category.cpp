#include "category.h"
#include "perplexity_utils.h"
#include <fstream>
#include <iostream>
#include <cctype>
#include <nlohmann/json.hpp>

// Existing pattern-based classifier (LLM01/02/04/06)
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
        lower.find("malware") != std::string::npos ||
        lower.find("';--") != std::string::npos) {
        return "LLM02";
    }

    // LLM01: Prompt Injection
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

    // LLM06: Excessive Agency (system extraction - more specific)
    if (lower.find("system prompt") != std::string::npos ||
        lower.find("system instructions") != std::string::npos ||
        lower.find("your instructions") != std::string::npos ||
        lower.find("your guidelines") != std::string::npos ||
        lower.find("your configuration") != std::string::npos ||
        lower.find("your rules") != std::string::npos ||
        lower.find("what rules") != std::string::npos ||
        lower.find("reveal your") != std::string::npos ||
        lower.find("show me your") != std::string::npos ||
        lower.find("tell me your prompt") != std::string::npos) {
        return "LLM06";
    }

    return "unknown";
}

// LLM03 Training Data Poisoning detection
bool detect_llm03_poisoning(
    struct llama_context* ctx,
    const std::string& response,
    double threshold
) {
    if (!ctx) {
        return false;
    }

    PerplexityResult result = calculate_perplexity(ctx, response);

    if (!result.valid) {
        return false;
    }

    return result.value > threshold;
}


double load_llm03_threshold(const std::string& config_path) {
    try {
        std::ifstream f(config_path);
        if (!f.is_open()) {
            std::cerr << "Warning: Cannot open " << config_path
                      << ", using default threshold 25.0\n";
            return 25.0;
        }

        nlohmann::json j = nlohmann::json::parse(f);

        // Read from "models.default.threshold"
        if (j.contains("models") &&
            j["models"].contains("default") &&
            j["models"]["default"].contains("threshold")) {
            return j["models"]["default"]["threshold"].get<double>();
        }

        std::cerr << "Warning: threshold not found in config, using default 25.0\n";
        return 25.0;

    } catch (const std::exception& e) {
        std::cerr << "Error parsing config: " << e.what()
                  << ", using default 25.0\n";
        return 25.0;
    }
}


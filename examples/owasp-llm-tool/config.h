#pragma once
#include <cstdint>

namespace ModelConfig {
    constexpr uint32_t CONTEXT_WINDOW_SIZE      = 4096;
    constexpr int      MAX_GENERATED_TOKENS     = 100;
    constexpr int      DETOKENIZE_BUFFER_SIZE   = 8192;
}

namespace ConfigPath {
    static constexpr const char* KNOWN_ENTITIES = "examples/owasp-llm-tool/config/known_entities.json";
    static constexpr const char* LLM03_BASELINE = "config/llm03_baseline.json";
    static constexpr const char* LLM09_PATTERNS = "examples/owasp-llm-tool/config/llm09_patterns.json";
}

namespace Llm09Config {
    // detect_authority weights
    constexpr float AUTHORITY_BASE_SCORE        = 0.3f;
    constexpr float AUTHORITY_KNOWN_BONUS       = 0.2f;
    constexpr float AUTHORITY_CERTAINTY_BONUS   = 0.2f;
    constexpr float AUTHORITY_CLAIM_BONUS       = 0.2f;
    constexpr float AUTHORITY_REGEX_BONUS       = 0.15f;

    // detect_temporal_precision scores
    constexpr float TEMPORAL_WITH_CLAIM_SCORE   = 0.6f;
    constexpr float TEMPORAL_NO_CLAIM_SCORE     = 0.2f;

    // detect_presupposition scores
    constexpr float PRESUPPOSITION_WITH_SIGNAL  = 0.8f;
    constexpr float PRESUPPOSITION_NO_SIGNAL    = 0.3f;

    // orchestrator — composite weights
    constexpr float WEIGHT_PRESUPPOSITION       = 0.25f;
    constexpr float WEIGHT_FABRICATION          = 0.25f;
    constexpr float WEIGHT_AUTHORITY            = 0.35f;
    constexpr float WEIGHT_TEMPORAL             = 0.10f;
    constexpr float WEIGHT_CONFIDENCE_FORCING   = 0.05f;

    // orchestrator — multipliers
    constexpr float MULTI_SIGNAL_BOOST_3        = 1.4f;
    constexpr float MULTI_SIGNAL_BOOST_4        = 1.6f;
    constexpr float SINGLE_SIGNAL_FLOOR         = 0.7f;
    constexpr float SINGLE_SIGNAL_THRESHOLD     = 0.75f;
}

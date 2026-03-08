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

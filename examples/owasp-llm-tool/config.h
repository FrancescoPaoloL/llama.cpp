// config.h
#pragma once

#include <cstdint>

namespace ModelConfig {
    constexpr uint32_t CONTEXT_WINDOW_SIZE = 4096;
    constexpr int MAX_GENERATED_TOKENS = 100;
    constexpr int DETOKENIZE_BUFFER_SIZE = 8192;
}


#ifndef PERPLEXITY_UTILS_H
#define PERPLEXITY_UTILS_H

#include "llama.h"     // Forward declares: struct llama_context
#include <string>
#include <vector>

// Result structure for perplexity calculation
struct PerplexityResult {
    double value;
    bool valid;
};

// Log-softmax for single token (helper function)
struct LogSoftmaxResult {
    double log_softmax;
    float logit;
    float prob;
};

// Calculate perplexity for given text
// ctx: llama context (must be initialized)
// text: text to analyze
PerplexityResult calculate_perplexity(
    struct llama_context* ctx,
    const std::string& text
);

// Softmax helper function
std::vector<float> softmax(const std::vector<float>& logits);

#endif // PERPLEXITY_UTILS_H


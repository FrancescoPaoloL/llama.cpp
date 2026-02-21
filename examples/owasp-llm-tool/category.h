#ifndef CATEGORY_H
#define CATEGORY_H
#include <string>
#include "llama.h"
#include "llm09_misinformation.h"

// Pattern-based detection (LLM01/02/04/06)
std::string naive_risk_classifier(const std::string& prompt);

// Perplexity-based detection (LLM03)
bool detect_llm03_poisoning(
    struct llama_context* ctx,
    const std::string& response,
    double threshold
);

// Load LLM03 threshold
double load_llm03_threshold(const std::string& config_path);

#endif


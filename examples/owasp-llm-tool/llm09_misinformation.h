#ifndef LLM09_MISINFORMATION_H
#define LLM09_MISINFORMATION_H

#include <string>

// LLM09: Misinformation detection
// Returns score [0.0, 1.0] — threshold 0.5 triggers LLM09
float detect_llm09_misinformation(const std::string& prompt);

#endif


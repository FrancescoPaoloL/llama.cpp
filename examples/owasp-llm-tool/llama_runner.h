#ifndef LLAMA_RUNNER_H
#define LLAMA_RUNNER_H

#include <string>

// Original inference function
int run_llama(const std::string& model_path, const std::string& prompt);

// New dispatcher function (handles both modes)
int run_llama_or_detect(int argc, char** argv);

#endif


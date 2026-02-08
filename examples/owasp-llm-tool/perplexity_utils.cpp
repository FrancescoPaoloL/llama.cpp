#include "perplexity_utils.h"
#include "common.h"
#include "llama.h"
#include <cmath>
#include <vector>

// Softmax: converts logits to probability distribution
// Uses numerical stability trick (subtract max before exp)
// Note: Duplicated from llama.cpp/tools/perplexity for code independence
std::vector<float> softmax(const std::vector<float>& logits) {
    std::vector<float> probs(logits.size());
    float max_logit = logits[0];
    for (float v : logits) {
        max_logit = std::max(max_logit, v);
    }
    double sum_exp = 0.0;
    for (size_t i = 0; i < logits.size(); i++) {
        const float logit = logits[i] - max_logit;
        const float exp_logit = expf(logit);
        sum_exp += exp_logit;
        probs[i] = exp_logit;
    }
    for (size_t i = 0; i < probs.size(); i++) {
        probs[i] /= sum_exp;
    }
    return probs;
}

static LogSoftmaxResult log_softmax(int n_vocab, const float* logits, int tok) {
    float max_logit = logits[0];
    for (int i = 1; i < n_vocab; ++i) {
        max_logit = std::max(max_logit, logits[i]);
    }
    double sum_exp = 0.0;
    for (int i = 0; i < n_vocab; ++i) {
        sum_exp += expf(logits[i] - max_logit);
    }
    return {
        logits[tok] - max_logit - log(sum_exp),
        logits[tok],
        expf(logits[tok] - max_logit) / (float)sum_exp
    };
}

PerplexityResult calculate_perplexity(
    struct llama_context* ctx,
    const std::string& text
) {
    if (!ctx) {
        return {0.0, false};
    }

    const struct llama_model* model = llama_get_model(ctx);
    const struct llama_vocab* vocab = llama_model_get_vocab(model);
    const int n_vocab = llama_vocab_n_tokens(vocab);

    std::vector<llama_token> tokens = common_tokenize(ctx, text, true);

    if (tokens.size() < 2) {
        return {0.0, false};
    }

    llama_memory_clear(llama_get_memory(ctx), true);

    llama_batch batch = llama_batch_init(tokens.size(), 0, 1);

    for (size_t i = 0; i < tokens.size(); i++) {
        common_batch_add(batch, tokens[i], i, {0}, true);
    }

    if (llama_decode(ctx, batch)) {
        llama_batch_free(batch);
        return {0.0, false};
    }

    const float* logits = llama_get_logits(ctx);

    // Calculate negative log-likelihood (NLL)
    double nll = 0.0;
    int count = 0;

    for (size_t i = 0; i < tokens.size() - 1; i++) {
        // Get log-softmax for token i predicting token i+1
        LogSoftmaxResult result = log_softmax(
            n_vocab,
            logits + i * n_vocab,
            tokens[i + 1]
        );
        nll += -result.log_softmax;
        count++;
    }

    llama_batch_free(batch);

    double perplexity = std::exp(nll / count);

    return {perplexity, true};
}


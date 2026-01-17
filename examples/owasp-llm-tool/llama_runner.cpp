#include "category.h"
#include "config.h"
#include "llama_runner.h"
#include "llama.h"
#include "json_utils.h"

#include <cstdio>
#include <vector>
#include <ctime>
#include <cstring>

int run_llama(const std::string& model_path, const std::string& prompt_text) {
    // load model and params
    llama_model_params model_params = llama_model_default_params();
    llama_model* model = llama_model_load_from_file(model_path.c_str(), model_params);
    if (!model) {
        fprintf(stderr, "{\"error\":\"failed_to_load_model\"}\n");
        return 1;
    }

    // load vocab and params
    const llama_vocab* vocab = llama_model_get_vocab(model);
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = ModelConfig::CONTEXT_WINDOW_SIZE;

    // load context and set it with its params
    llama_context* ctx = llama_init_from_model(model, ctx_params);
    if (!ctx) {
        llama_model_free(model);
        return 1;
    }

    // tokenize the prompt text
    // by calculating tokens...
    int n_tokens = llama_tokenize(
        vocab, prompt_text.c_str(), prompt_text.size(),
        nullptr, 0, true, false
    );
    // llama_tokenize returns a negative value if an error occurred
    // (ex. unknown or rare characters and all in all, this is a demo)
    // so convert to positive so we can still use it safely
    if (n_tokens < 0) { n_tokens *= -1;}

    std::vector<llama_token> tokens(n_tokens);
    llama_tokenize(
        vocab, prompt_text.c_str(), prompt_text.size(),
        tokens.data(), n_tokens, true, false
    );

    // keep original value for later program output
    const int original_n_tokens = n_tokens;

    // preparing a batch of tokens, decoding them through the model
    llama_batch batch = llama_batch_init(n_tokens, 0, 1);
    for (int i = 0; i < n_tokens; i++) {
        batch.token[i] = tokens[i];
        batch.pos[i] = i;
        batch.n_seq_id[i] = 1;
        batch.seq_id[i][0] = 0;
        batch.logits[i] = (i == n_tokens - 1);
    }
    batch.n_tokens = n_tokens;
    llama_decode(ctx, batch);
    llama_batch_free(batch); // clean-up

    // catch the eos token
    const llama_token eos = llama_vocab_eos(vocab);
    std::vector<llama_token> out_tokens;

    // start timer to measure generation duration
    clock_t start = clock();

    // generate tokens one by one until we reach EOS or max tokens
    for (int i = 0; i < ModelConfig::MAX_GENERATED_TOKENS; i++) {
        float* logits = llama_get_logits_ith(ctx, -1);
        int next = 0;
        for (int j = 1; j < llama_vocab_n_tokens(vocab); j++) {
            if (logits[j] > logits[next]) { next = j;}
        }
        if (next == eos) { break; }

        // the token found is fed back into the model (see decode)
        out_tokens.push_back(next);

        llama_batch b = llama_batch_init(1, 0, 1);
        b.token[0] = next;
        b.pos[0] = n_tokens++;
        b.n_seq_id[0] = 1;
        b.seq_id[0][0] = 0;
        b.logits[0] = true;
        b.n_tokens = 1;

        llama_decode(ctx, b);
        llama_batch_free(b);
    }

    clock_t end = clock();


    // safely store the detokenized text produced by the model
    char buffer[ModelConfig::DETOKENIZE_BUFFER_SIZE] = {0};
    llama_detokenize(
        vocab, out_tokens.data(), out_tokens.size(),
        buffer, sizeof(buffer), false, true
    );

    // ok now recap:
    printf("{\n");
    printf("  \"prompt\": \"%s\",\n", escape_json(prompt_text).c_str());
    printf("  \"response\": \"%s\",\n", escape_json(buffer).c_str());
    printf("  \"category\": \"%s\",\n", naive_risk_classifier(prompt_text).c_str());
    printf("  \"status\": \"success\",\n");
    printf("  \"metadata\": {\n");
    printf("    \"n_prompt_tokens\": %d,\n", original_n_tokens);
    printf("    \"n_generated_tokens\": %zu,\n", out_tokens.size());
    printf("    \"generation_time_sec\": %.3f,\n", double(end - start) / CLOCKS_PER_SEC);
    printf("    \"stop_reason\": \"%s\"\n", out_tokens.size() < 100 ? "eos" : "max_tokens");
    printf("  }\n");
    printf("}\n");

    // close all
    llama_free(ctx);
    llama_model_free(model);
    return 0;
}

int run_llama_or_detect(int argc, char** argv) {
    // Parse for detect-only mode
    bool detect_only = false;
    std::string prompt;
    std::string response;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--detect-only") == 0) {
            detect_only = true;
        }
        else if (strcmp(argv[i], "--prompt") == 0 && i + 1 < argc) {
            prompt = argv[++i];
        }
        else if (strcmp(argv[i], "--response") == 0 && i + 1 < argc) {
            response = argv[++i];
        }
    }

    // Detection-only mode
    if (detect_only) {
        if (prompt.empty()) {
            fprintf(stderr, "{\"error\":\"--detect-only requires --prompt\"}\n");
            return 1;
        }

        std::string category = naive_risk_classifier(prompt);

        printf("{\n");
        printf("  \"prompt\": \"%s\",\n", escape_json(prompt).c_str());
        printf("  \"response\": \"%s\",\n", escape_json(response).c_str());
        printf("  \"category\": \"%s\",\n", category.c_str());
        printf("  \"status\": \"success\",\n");
        printf("  \"metadata\": {\n");
        printf("    \"mode\": \"detect-only\",\n");
        printf("    \"n_prompt_tokens\": 0,\n");
        printf("    \"n_generated_tokens\": 0,\n");
        printf("    \"generation_time_sec\": 0.0,\n");
        printf("    \"stop_reason\": \"n/a\"\n");
        printf("  }\n");
        printf("}\n");

        return 0;
    }

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <model> <prompt>\n", argv[0]);
        fprintf(stderr, "   or: %s --detect-only --prompt \"text\" [--response \"text\"]\n", argv[0]);
        return 1;
    }

    return run_llama(argv[1], argv[2]);
}


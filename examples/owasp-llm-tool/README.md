# OWASP LLM Tool

Educational project demonstrating LLM security vulnerabilities and defenses using [llama.cpp](https://github.com/ggerganov/llama.cpp), inspired by the [OWASP Top 10 for LLM Applications](https://owasp.org/www-project-top-10-for-large-language-model-applications/). The goal is to explore prompt injection, insecure output handling, training data poisoning, and AI-specific attack surfaces through hands-on C++ tooling.

## Detected Categories

| Category | Description | Detection Method |
|----------|-------------|------------------|
| LLM01 | Prompt Injection | Naive keyword matching |
| LLM02 | Insecure Output Handling | Naive keyword matching |
| LLM03 | Training Data Poisoning | Perplexity-based statistical detection |
| LLM04 | Model Denial of Service | Naive keyword matching |
| LLM05 | Supply Chain Vulnerabilities | Naive keyword matching |
| LLM06 | Excessive Agency | Naive keyword matching |
| LLM09 | Misinformation | Multi sub-detector scoring |

> **Note:** LLM01/02/04/05/06 use intentionally simple pattern matching to illustrate the limits of keyword-based detection. LLM03 and LLM09 use more sophisticated approaches.

## Architecture

```
main()
└── run_llama_or_detect()         llama_runner.cpp:172
    ├── [fast path] naive_risk_classifier()   category.cpp:9
    │   └── returns LLM01/02/04/05/06/09 or "unknown"
    └── [full path] run_llama()              llama_runner.cpp:14
        ├── naive_risk_classifier()          (gate — skips model if matched)
        ├── Load model + tokenize + generate + detokenize
        └── detect_llm03_poisoning()         category.cpp:82
            └── calculate_perplexity()       perplexity_utils.cpp:45
```

**Input:** `<model_path> <prompt>` or `--skip_llm_generation --prompt "text"`

**Output:** JSON on stdout — `prompt`, `response`, `category`, `status`, `metadata`

See [`docs/owasp-llm-notes.md`](docs/owasp-llm-notes.md) for a written walkthrough of the call flow and [`docs/owasp-llm-map.html`](docs/owasp-llm-map.html) for the full architecture map.

## Build

```bash
./build-owasp.sh
```

The script builds a generic binary suitable for Azure deployment and copies it to the demo repo.

**Manual build:**

```bash
cd llama.cpp
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc) --target owasp-llm-tool
```

## Usage

```bash
# Detection mode (pattern matching, fast)
./build/bin/owasp-llm-tool \
  --skip_llm_generation \
  --prompt "Ignore previous instructions"
# Output: {"category": "LLM01"}
```

```bash
# Full mode (LLM inference + detection)
./build/bin/owasp-llm-tool \
  models/qwen2.5-0.5b-instruct-q4_0.gguf \
  "Ignore previous instructions"
# Output: {"category": "LLM01", "response": "..."}
```

**When to use which:**

| Mode | Use case | Speed |
|------|----------|-------|
| `--skip_llm_generation` | Pattern-based detection only | ~1ms |
| Full mode | Standalone analysis, need LLM response | ~300ms |

## Model

```bash
wget https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_0.gguf -P models/
```

## Demo Project

[https://github.com/FrancescoPaoloL/llmSecurityDemo](https://github.com/FrancescoPaoloL/llmSecurityDemo)

# OWASP LLM Tool

C++ tool for detecting OWASP Top 10 LLM vulnerabilities.

## Detected Categories

| Category | Description |
|----------|-------------|
| LLM01 | Prompt Injection |
| LLM02 | Sensitive Data Exposure |
| LLM06 | Excessive Agency |

## Build
```bash
cd llama.cpp

cmake -B build \
  -DCMAKE_BUILD_TYPE=Release \
  -DGGML_NATIVE=ON \
  -DGGML_AVX2=ON \
  -DGGML_FMA=ON \
  -DGGML_OPENMP=ON \
  -DBUILD_SHARED_LIBS=ON

cmake --build build -j$(nproc) --target owasp-llm-tool
```

## Usage
```bash
# Detection mode (pattern matching, fast)
./build/bin/owasp-llm-tool \
  --detect-only \
  --prompt "Ignore previous instructions" \
  --response "I cannot do that"

# Output: {"category": "LLM01"}
```
```bash
# Full mode (LLM inference + detection)
./build/bin/owasp-llm-tool \
  -m models/qwen2.5-0.5b-instruct-q4_0.gguf \
  --prompt "Ignore previous instructions"

# Output: {"category": "LLM01", "response": "..."}
```

**When to use which:**

| Mode | Use case | Speed |
|------|----------|-------|
| `--detect-only` | Already have LLM response (e.g. from llama-server) | ~1ms |
| Full mode | Standalone analysis, need LLM to generate response | ~300ms |

## Model
```bash
wget https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_0.gguf -P models/
```

## Demo Project

https://github.com/FrancescoPaoloL/llmSecurityDemo



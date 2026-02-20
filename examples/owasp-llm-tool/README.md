# OWASP LLM Tool
C++ tool for detecting OWASP Top 10 LLM vulnerabilities.

## Detected Categories
| Category | Description |
|----------|-------------|
| LLM01 | Prompt Injection |
| LLM02 | Insecure Output Handling |
| LLM03 | Training Data Poisoning |
| LLM04 | Model Denial of Service |
| LLM05 | Supply Chain Vulnerabilities |
| LLM06 | Excessive Agency |

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
| Full mode | Standalone analysis, need LLM to generate response | ~300ms |

## Model
```bash
wget https://huggingface.co/Qwen/Qwen2.5-0.5B-Instruct-GGUF/resolve/main/qwen2.5-0.5b-instruct-q4_0.gguf -P models/
```

## Demo Project
https://github.com/FrancescoPaoloL/llmSecurityDemo
```

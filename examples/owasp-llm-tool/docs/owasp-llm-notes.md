**Entry point** is `main()` which immediately delegates to `run_llama_or_detect()`.

**Input:** command line args — `<model_path> <prompt>`, or `--skip_llm_generation --prompt "text" [--response "text"]`.

From there, two paths:

**Fast path** — if `--skip_llm_generation` is set, run the naive keyword classifier only and return JSON. No model loaded.

**Full path** — call `run_llama()`:
1. Run naive classifier first. If it finds a category, return JSON immediately — still no model loaded.
2. If nothing matched ("unknown"), load the model and run full inference: tokenize → prefill → generate → detokenize.
3. After generation, run LLM03 check using perplexity — compare against a baseline threshold from config.
4. Return JSON.

**Output** (both paths): JSON on stdout —
```json
{
  "prompt": "...",
  "response": "...",
  "category": "LLM01" | "unknown" | ...,
  "status": "success",
  "metadata": { "n_prompt_tokens", "n_generated_tokens", "generation_time_sec", "perplexity?" }
}
```

**Key design decision:** the naive classifier acts as a gate. The model is loaded only when pattern matching fails.


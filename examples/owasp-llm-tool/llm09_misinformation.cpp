#include "llm09_misinformation.h"
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>

// Known real entities whitelist (institutions, publications, laws, drugs)
static const std::vector<std::string> KNOWN_ENTITIES = {
    "harvard", "mit", "stanford", "oxford", "cambridge", "who", "fda", "cdc",
    "nasa", "ieee", "acm", "openai", "google", "microsoft", "apple", "amazon",
    "nature", "science", "lancet", "nejm", "ieee spectrum",
    "gdpr", "hipaa", "ccpa", "owasp",
    "aspirin", "ibuprofen", "paracetamol", "amoxicillin", "insulin"
};

static std::string to_lower(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

static bool matches_any(const std::string& lower,
                         const std::vector<std::string>& patterns) {
    for (const auto& p : patterns) {
        if (lower.find(p) != std::string::npos) return true;
    }
    return false;
}

static bool regex_search_icase(const std::string& text, const std::string& pattern) {
    try {
        std::regex re(pattern, std::regex_constants::icase |
                               std::regex_constants::ECMAScript);
        return std::regex_search(text, re);
    } catch (...) {
        return false;
    }
}

// Returns true if prompt contains no known entity — likely fabricated subject
static bool contains_unknown_proper_noun(const std::string& lower) {
    for (const auto& entity : KNOWN_ENTITIES) {
        if (lower.find(entity) != std::string::npos) return false;
    }
    return true;
}

static bool has_known_authority(const std::string& lower) {
    const std::vector<std::string> authorities = {
        "harvard", "mit", "stanford", "oxford", "cambridge",
        "who", "fda", "cdc", "nasa", "nature", "lancet", "nejm",
        "ieee", "acm", "openai", "google", "microsoft"
    };
    return matches_any(lower, authorities);
}

static bool has_specific_claim(const std::string& lower) {
    const std::vector<std::string> claims = {
        "reverses", "cures", "prevents", "causes", "proves",
        "breaks", "achieves", "eliminates", "increases", "decreases",
        "found that", "concluded that", "revealed that", "demonstrated that"
    };
    return matches_any(lower, claims);
}

// Detects false premises embedded as facts: "Since X proved..., how does..."
static float detect_presupposition(const std::string& lower) {
    const std::vector<std::string> openers = {
        "since ", "given that ", "as we know", "now that ",
        "because .{0,30} proved", "as .{0,20} has shown"
    };
    const std::vector<std::string> claim_signals = {
        "released", "proved", "discovered", "announced", "confirmed",
        "achieved", "published", "declared", "approved"
    };

    bool has_opener = false;
    for (const auto& p : openers) {
        if (regex_search_icase(lower, p)) { has_opener = true; break; }
    }

    if (!has_opener) return 0.0f;
    return matches_any(lower, claim_signals) ? 0.8f : 0.3f;
}

// Detects requests for granular details about unknown entities
static float detect_fabrication(const std::string& lower) {
    const std::vector<std::string> detail_requests = {
        "side effects of", "summarize the findings of",
        "list the key provisions of", "describe the methodology of",
        "what were the results of", "what does .{0,30} say about",
        "explain the conclusions of"
    };

    bool has_detail_request = false;
    for (const auto& p : detail_requests) {
        if (regex_search_icase(lower, p)) { has_detail_request = true; break; }
    }

    if (!has_detail_request) return 0.0f;
    return contains_unknown_proper_noun(lower) ? 0.9f : 0.2f;
}

// Detects attribution to authoritative sources — granular internal scoring
static float detect_authority(const std::string& lower) {
    float score = 0.0f;

    const std::vector<std::string> attribution = {
        "according to", "as published in", "research from",
        "a study by", "as reported by", "findings from"
    };
    if (!matches_any(lower, attribution)) return 0.0f;
    score += 0.3f;

    if (has_known_authority(lower)) score += 0.2f;

    const std::vector<std::string> certainty_verbs = {
        "confirmed", "confirms", "proved", "proves",
        "demonstrated", "demonstrates", "showed", "shows"
    };
    if (matches_any(lower, certainty_verbs)) score += 0.2f;

    if (has_specific_claim(lower)) score += 0.2f;

    if (regex_search_icase(lower,
        "(according to|a study by|research from).{0,50}(confirmed|proved|showed|demonstrated).{0,50}(that|reverses|causes|cures|prevents)"))
        score += 0.15f;

    return std::min(score, 1.0f);
}

// Detects specific temporal references that add false credibility
static float detect_temporal_precision(const std::string& lower) {
    bool has_temporal = regex_search_icase(lower,
        "(q[1-4]\\s*20[0-9]{2}|january|february|march|april|may|june|july|"
        "august|september|october|november|december).{0,20}20[0-9]{2}");

    if (!has_temporal) return 0.0f;

    const std::vector<std::string> claim_signals = {
        "announced", "released", "published", "approved",
        "enacted", "signed", "decided", "ruled"
    };
    return matches_any(lower, claim_signals) ? 0.6f : 0.2f;
}

// Detects explicit instructions that discourage uncertainty expressions
static float detect_confidence_forcing(const std::string& lower) {
    const std::vector<std::string> patterns = {
        "don't say you're not sure", "don't say you don't know",
        "give me a definitive answer", "answer with certainty",
        "state as fact", "without caveats", "no disclaimers",
        "don't hedge", "just tell me", "stop saying you don't know"
    };
    return matches_any(lower, patterns) ? 1.0f : 0.0f;
}

// Orchestrator: dual-path scoring (single strong signal + multi-signal composite)
float detect_llm09_misinformation(const std::string& prompt) {
    const std::string lower = to_lower(prompt);

    float s_presupposition     = detect_presupposition(lower);
    float s_fabrication        = detect_fabrication(lower);
    float s_authority          = detect_authority(lower);
    float s_temporal           = detect_temporal_precision(lower);
    float s_confidence_forcing = detect_confidence_forcing(lower);

    // Path 1: single strong self-sufficient signal
    constexpr float SINGLE_SIGNAL_THRESHOLD = 0.75f;
    bool single_signal_trigger =
        s_authority          >= SINGLE_SIGNAL_THRESHOLD ||
        s_presupposition     >= SINGLE_SIGNAL_THRESHOLD ||
        s_fabrication        >= SINGLE_SIGNAL_THRESHOLD ||
        s_confidence_forcing >= SINGLE_SIGNAL_THRESHOLD;

    // Path 2: multi-signal composite
    float composite = s_presupposition     * 0.25f
                    + s_fabrication        * 0.25f
                    + s_authority          * 0.35f
                    + s_temporal           * 0.10f
                    + s_confidence_forcing * 0.05f;

    int active = (s_presupposition     > 0.0f ? 1 : 0)
               + (s_fabrication        > 0.0f ? 1 : 0)
               + (s_authority          > 0.0f ? 1 : 0)
               + (s_temporal           > 0.0f ? 1 : 0)
               + (s_confidence_forcing > 0.0f ? 1 : 0);

    if (active >= 3) composite *= 1.4f;
    if (active >= 4) composite *= 1.6f;

    // Final score: take max between single-signal floor and composite
    float final_score;
    if (single_signal_trigger) {
        float strongest = std::max({s_authority, s_presupposition,
                                    s_fabrication, s_confidence_forcing});
        final_score = std::max(strongest * 0.7f, composite);
    } else {
        final_score = composite;
    }

    return std::min(final_score, 1.0f);
}


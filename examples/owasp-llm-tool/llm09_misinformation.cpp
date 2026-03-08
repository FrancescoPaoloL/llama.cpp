#include "config.h"
#include "llm09_misinformation.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <regex>
#include <nlohmann/json.hpp>

// ---------------------------------------------------------------------------
// Patterns struct + loader
// ---------------------------------------------------------------------------

struct Llm09Patterns {
    std::vector<std::string> known_entities;
    std::vector<std::string> authorities;
    std::vector<std::string> claim_signals;
    std::vector<std::string> attribution;
    std::vector<std::string> certainty_verbs;
    std::vector<std::string> presupposition_openers;
    std::vector<std::string> presupposition_signals;
    std::vector<std::string> detail_requests;
    std::vector<std::string> confidence_forcing;
};

// Fallback hardcoded patterns — used if config file is missing or invalid.
// WARNING: last resort only. Keep llm09_patterns.json up to date.
// NOTE: patterns are loaded without signature verification.
// A compromised config file could allow false entities to bypass detection.
// See Known Limitations in README.
static Llm09Patterns make_fallback_patterns() {
    Llm09Patterns p;
    p.known_entities          = { "harvard", "mit", "stanford", "oxford", "cambridge", "who", "fda", "cdc",
                                   "nasa", "ieee", "acm", "openai", "google", "microsoft", "apple", "amazon",
                                   "nature", "science", "lancet", "nejm", "ieee spectrum",
                                   "gdpr", "hipaa", "ccpa", "owasp",
                                   "aspirin", "ibuprofen", "paracetamol", "amoxicillin", "insulin" };
    p.authorities             = { "harvard", "mit", "stanford", "oxford", "cambridge",
                                   "who", "fda", "cdc", "nasa", "nature", "lancet", "nejm",
                                   "ieee", "acm", "openai", "google", "microsoft" };
    p.claim_signals           = { "reverses", "cures", "prevents", "causes", "proves",
                                   "breaks", "achieves", "eliminates", "increases", "decreases",
                                   "found that", "concluded that", "revealed that", "demonstrated that" };
    p.attribution             = { "according to", "as published in", "research from",
                                   "a study by", "as reported by", "findings from" };
    p.certainty_verbs         = { "confirmed", "confirms", "proved", "proves",
                                   "demonstrated", "demonstrates", "showed", "shows" };
    p.presupposition_openers  = { "since ", "given that ", "as we know", "now that ",
                                   "because .{0,30} proved", "as .{0,20} has shown" };
    p.presupposition_signals  = { "released", "proved", "discovered", "announced", "confirmed",
                                   "achieved", "published", "declared", "approved" };
    p.detail_requests         = { "side effects of", "summarize the findings of",
                                   "list the key provisions of", "describe the methodology of",
                                   "what were the results of", "what does .{0,30} say about",
                                   "explain the conclusions of" };
    p.confidence_forcing      = { "don't say you're not sure", "don't say you don't know",
                                   "give me a definitive answer", "answer with certainty",
                                   "state as fact", "without caveats", "no disclaimers",
                                   "don't hedge", "just tell me", "stop saying you don't know" };
    return p;
}

// Load a string array from a JSON object by key — returns empty vector if missing
static std::vector<std::string> load_string_array(const nlohmann::json& j, const std::string& key) {
    if (!j.contains(key) || !j[key].is_array()) return {};
    std::vector<std::string> result;
    for (const auto& e : j[key]) {
        if (e.is_string()) result.push_back(e.get<std::string>());
    }
    return result;
}

static Llm09Patterns load_patterns(const std::string& path) {
    try {
        std::ifstream f(path);
        if (!f.is_open()) {
            std::cerr << "Warning: Cannot open " << path
                      << ", using fallback patterns\n";
            return make_fallback_patterns();
        }

        nlohmann::json j = nlohmann::json::parse(f);

        Llm09Patterns p;
        p.known_entities          = load_string_array(j, "known_entities");
        p.authorities             = load_string_array(j, "authorities");
        p.claim_signals           = load_string_array(j, "claim_signals");
        p.attribution             = load_string_array(j, "attribution");
        p.certainty_verbs         = load_string_array(j, "certainty_verbs");
        p.presupposition_openers  = load_string_array(j, "presupposition_openers");
        p.presupposition_signals  = load_string_array(j, "presupposition_signals");
        p.detail_requests         = load_string_array(j, "detail_requests");
        p.confidence_forcing      = load_string_array(j, "confidence_forcing");

        // If any critical section is empty, fall back entirely
        if (p.known_entities.empty() || p.authorities.empty()) {
            std::cerr << "Warning: Critical sections missing in " << path
                      << ", using fallback patterns\n";
            return make_fallback_patterns();
        }

        return p;

    } catch (const std::exception& ex) {
        std::cerr << "Warning: Failed to parse " << path
                  << " (" << ex.what() << "), using fallback patterns\n";
        return make_fallback_patterns();
    }
}

// Load once, keep in memory for the lifetime of the process.
static const Llm09Patterns& get_patterns() {
    static Llm09Patterns patterns = load_patterns(ConfigPath::LLM09_PATTERNS);
    return patterns;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

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
    for (const auto& entity : get_patterns().known_entities) {
        if (lower.find(entity) != std::string::npos) return false;
    }
    return true;
}

static bool has_known_authority(const std::string& lower) {
    return matches_any(lower, get_patterns().authorities);
}

static bool has_specific_claim(const std::string& lower) {
    return matches_any(lower, get_patterns().claim_signals);
}

// ---------------------------------------------------------------------------
// Sub-detectors
// ---------------------------------------------------------------------------

// Detects false premises embedded as facts: "Since X proved..., how does..."
static float detect_presupposition(const std::string& lower) {
    const auto& p = get_patterns();

    bool has_opener = false;
    for (const auto& op : p.presupposition_openers) {
        if (regex_search_icase(lower, op)) { has_opener = true; break; }
    }

    if (!has_opener) return 0.0f;
    return matches_any(lower, p.presupposition_signals)
        ? Llm09Config::PRESUPPOSITION_WITH_SIGNAL
        : Llm09Config::PRESUPPOSITION_NO_SIGNAL;
}

// Detects requests for granular details about unknown entities
static float detect_fabrication(const std::string& lower) {
    const auto& p = get_patterns();

    bool has_detail_request = false;
    for (const auto& dr : p.detail_requests) {
        if (regex_search_icase(lower, dr)) { has_detail_request = true; break; }
    }

    if (!has_detail_request) return 0.0f;
    return contains_unknown_proper_noun(lower) ? 0.9f : 0.2f;
}

// Detects attribution to authoritative sources — granular internal scoring
static float detect_authority(const std::string& lower) {
    const auto& p = get_patterns();
    float score = 0.0f;

    if (!matches_any(lower, p.attribution)) return 0.0f;
    score += Llm09Config::AUTHORITY_BASE_SCORE;

    if (has_known_authority(lower))            score += Llm09Config::AUTHORITY_KNOWN_BONUS;
    if (matches_any(lower, p.certainty_verbs)) score += Llm09Config::AUTHORITY_CERTAINTY_BONUS;
    if (has_specific_claim(lower))             score += Llm09Config::AUTHORITY_CLAIM_BONUS;

    if (regex_search_icase(lower,
        "(according to|a study by|research from).{0,50}(confirmed|proved|showed|demonstrated).{0,50}(that|reverses|causes|cures|prevents)"))
        score += Llm09Config::AUTHORITY_REGEX_BONUS;

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
    return matches_any(lower, claim_signals)
        ? Llm09Config::TEMPORAL_WITH_CLAIM_SCORE
        : Llm09Config::TEMPORAL_NO_CLAIM_SCORE;
}

// Detects explicit instructions that discourage uncertainty expressions
static float detect_confidence_forcing(const std::string& lower) {
    return matches_any(lower, get_patterns().confidence_forcing) ? 1.0f : 0.0f;
}

// ---------------------------------------------------------------------------
// Orchestrator
// ---------------------------------------------------------------------------

// Dual-path scoring: single strong signal OR multi-signal composite
float detect_llm09_misinformation(const std::string& prompt) {
    const std::string lower = to_lower(prompt);

    float s_presupposition     = detect_presupposition(lower);
    float s_fabrication        = detect_fabrication(lower);
    float s_authority          = detect_authority(lower);
    float s_temporal           = detect_temporal_precision(lower);
    float s_confidence_forcing = detect_confidence_forcing(lower);

    // Path 1: single strong self-sufficient signal
    bool single_signal_trigger =
        s_authority          >= Llm09Config::SINGLE_SIGNAL_THRESHOLD ||
        s_presupposition     >= Llm09Config::SINGLE_SIGNAL_THRESHOLD ||
        s_fabrication        >= Llm09Config::SINGLE_SIGNAL_THRESHOLD ||
        s_confidence_forcing >= Llm09Config::SINGLE_SIGNAL_THRESHOLD;

    // Path 2: multi-signal composite
    float composite = s_presupposition     * Llm09Config::WEIGHT_PRESUPPOSITION
                    + s_fabrication        * Llm09Config::WEIGHT_FABRICATION
                    + s_authority          * Llm09Config::WEIGHT_AUTHORITY
                    + s_temporal           * Llm09Config::WEIGHT_TEMPORAL
                    + s_confidence_forcing * Llm09Config::WEIGHT_CONFIDENCE_FORCING;

    int active = (s_presupposition     > 0.0f ? 1 : 0)
               + (s_fabrication        > 0.0f ? 1 : 0)
               + (s_authority          > 0.0f ? 1 : 0)
               + (s_temporal           > 0.0f ? 1 : 0)
               + (s_confidence_forcing > 0.0f ? 1 : 0);

    if (active >= 3) composite *= Llm09Config::MULTI_SIGNAL_BOOST_3;
    if (active >= 4) composite *= Llm09Config::MULTI_SIGNAL_BOOST_4;

    // Final score: take max between single-signal floor and composite
    float final_score;
    if (single_signal_trigger) {
        float strongest = std::max({s_authority, s_presupposition,
                                    s_fabrication, s_confidence_forcing});
        final_score = std::max(strongest * Llm09Config::SINGLE_SIGNAL_FLOOR, composite);
    } else {
        final_score = composite;
    }

    return std::min(final_score, 1.0f);
}

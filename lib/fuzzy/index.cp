// Source: cp/lib/fuzzy/index.cp (Unified Exports for cap:fuzzy)
// Pure CP Fuzzy String Matching, Edit Distance, Jaro-Winkler & Ranked Search (`cap:fuzzy` & `cap.fuzzy`).

export {
    FuzzyError,
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaro,
    jaroWinkler,
    fuzzyScore,
    rankCandidates,
    freeMatches,
} from "./fuzzy.cp";

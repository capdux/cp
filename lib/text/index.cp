// Source: cp/lib/text/index.cp (Unified Exports for cap:text)
// Unified Public Exports for AI Text Chunking, Fuzzy Matching & LLM Token Budgeting (cap:text & cap.text)

export {
    TextErr,
    TextChunk,
    ChunkOptions,
    TextChunker,
} from "./chunker.cp";

export {
    FuzzyMatch,
    levenshtein,
    damerauLevenshtein,
    jaroWinkler,
    fuzzyScore,
    fuzzyTopK,
} from "./fuzzy.cp";

export {
    TextStats,
    TokenEstimator,
} from "./tokens.cp";

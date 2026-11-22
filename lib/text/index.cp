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

export {
    minCommonIndent,
    dedent,
    indentLines,
    stripCommonPrefix,
} from "./indent.cp";

export {
    wrapText,
    indentNonEmptyLines,
    commonIndent,
    dedentLines,
    truncateEllipsis,
} from "./wrap.cp";

export {
    WordIter,
    toSnakeCase,
    toScreamingSnakeCase,
    toKebabCase,
    toCamelCase,
    toPascalCase,
} from "./caseconv.cp";

export {
    FoldInfo,
    caseFold,
    caseFoldEqual,
} from "./casefold.cp";

export {
    RagChunkConfig,
    RagChunkSpan,
    chunkDocumentRag,
    estimateRagTokens,
    locateLineRange,
} from "./rag_chunker.cp";

export {
    BpeMerge,
    SpecialToken,
    BpeVocab,
    bpeEncodeSegment,
    bpeEncodeWithSpecials,
    bpeDecode,
    bpeCountTokens,
} from "./bpe.cp";

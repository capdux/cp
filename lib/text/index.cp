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

export {
    WordPieceConfig,
    WordPieceVocab,
    EncodedSequenceStats,
    wordPieceEncodeWord,
    wordPieceEncodeText,
    wordPieceEncodePair,
} from "./wordpiece.cp";

export {
    UnigramPiece,
    UnigramVocab,
    UnigramResult,
    unigramEncodeViterbi,
    unigramDecode,
    normalizeSentencePiece,
} from "./unigram.cp";

export {
    MdSectionKind,
    MdChunkConfig,
    MdHeadingTrail,
    MdChunkSpan,
    chunkMarkdownStructured,
} from "./markdown_chunk.cp";

export {
    DiffHunkHeader,
    DiffLineKind,
    DiffOp,
    PatchApplyStatus,
    PatchApplyResult,
    parseHunkHeader,
    parseDiffOp,
    applyUnifiedPatch,
} from "./unified_diff.cp";

export {
    SymbolKind,
    CodeSymbolSpan,
    extractCodeOutline,
    findSymbolByName,
    formatRepoMapLine,
} from "./code_outline.cp";

export {
    CodeTokenKind,
    CodeToken,
    MetaCapture,
    PatternMatchHit,
    tokenizeCodeLite,
    findCodePatternMatches,
} from "./code_pattern.cp";

// ============================================================================
// Pure-CP Templates, NLP, Full-Text Search, String Algorithms & Formatting
// ============================================================================
export {
    MustacheTokenKind,
    MustacheToken,
    MustacheVar,
    MustacheIter,
    validateMustache,
    renderMustache,
} from "./mustache.cp";

export * from "./template_value.cp";
export * from "./template_engine.cp";

export {
    SOUNDEX_LEN,
    MAX_PHONETIC_WORD_LEN,
    soundex,
    soundexEquals,
    soundexDifference,
    metaphone,
    nysiis,
    colognePhonetic,
} from "./soundex.cp";

export {
    PORTER_MAX_WORD_LEN,
    porterMeasure,
    porterStem,
    porterStemEquals,
} from "./porter.cp";

export {
    TokenizerConfig,
    defaultStopwords,
    tokenizeTerms,
    freeTerms,
} from "./bm25_tokenizer.cp";

export {
    Bm25Error,
    Bm25Config,
    Posting,
    Bm25Hit,
    Bm25Index,
} from "./bm25.cp";

export {
    RankedDoc,
    rrfFuse,
} from "./bm25_rrf.cp";

export {
    TextAlign,
    TableError,
    DisplayWidth,
} from "./table_width.cp";

export {
    TableStyle,
    ColumnSpec,
    Table,
} from "./table.cp";

export {
    Diff3Error,
    ConflictStyle,
    ConflictBlock,
    MergeHunk,
    MergeOptions,
    MergeResult,
    mergeLines,
    mergeTexts,
    hasConflicts,
} from "./diff3.cp";

export {
    MAX_NFA_STATES,
    RegexError,
    MatchSpan,
    NfaRegex,
} from "./nfa_regex.cp";

export {
    AhoError,
    MatchKind,
    AhoMatch,
    AhoBuilder,
    AhoCorasick,
} from "./aho.cp";

export * from "./kmp_z_algo.cp";

export {
    SuffixArrayError,
    SuffixArray,
} from "./suffixarray.cp";

export {
    SpellError,
    SuggestVerbosity,
    SpellSuggestion,
    damerauLevenshtein as spellDamerauLevenshtein,
    soundex as spellSoundex,
    SymSpellIndex,
    BkTree,
} from "./spell.cp";

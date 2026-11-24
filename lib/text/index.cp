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
} from "./ragChunker.cp";

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
} from "./markdownChunk.cp";

export {
    UNIFIED_DIFF_MAX_HUNK_LINES,
    DiffHunkHeader,
    DiffLineKind,
    DiffOp,
    PatchApplyStatus,
    PatchApplyResult,
    UnifiedDiffError,
    parseHunkHeader,
    parseDiffOp,
    applyUnifiedPatch,
    applyUnifiedDiff,
    generateUnifiedDiff,
} from "./unifiedDiff.cp";

export {
    SymbolKind,
    CodeSymbolSpan,
    extractCodeOutline,
    findSymbolByName,
    formatRepoMapLine,
} from "./codeOutline.cp";

export {
    CodeTokenKind,
    CodeToken,
    MetaCapture,
    PatternMatchHit,
    tokenizeCodeLite,
    findCodePatternMatches,
} from "./codePattern.cp";

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

export * from "./templateValue.cp";
export * from "./templateEngine.cp";

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
} from "./bm25Tokenizer.cp";

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
} from "./bm25Rrf.cp";

export {
    TextAlign,
    TableError,
    DisplayWidth,
} from "./tableWidth.cp";

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
} from "./nfaRegex.cp";

export {
    AhoError,
    MatchKind,
    AhoMatch,
    AhoBuilder,
    AhoCorasick,
} from "./aho.cp";

export * from "./kmpZAlgo.cp";

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

// ============================================================================
// Batch 34: I18n, Intl, IDNA, Highlight, Globset, Markup Diagrams & Text Formats
// ============================================================================
// --- i18n ---
export {
    I18nError,
    PluralCategory,
    I18nParam,
    LocaleNegotiator,
    PluralRules,
    I18nCatalog,
} from "./i18n.cp";

// --- intl ---
export {
    IntlError,
    NumberFormatOptions,
    NumberFormatter,
} from "./intlNumber.cp";

export {
    UnitError,
    ByteSize,
    HumanDuration,
    RelativeTime,
} from "./intlUnits.cp";

// --- idna ---
export {
    IdnaError,
    PUNYCODE_BASE,
    PUNYCODE_TMIN,
    PUNYCODE_TMAX,
    PUNYCODE_SKEW,
    PUNYCODE_DAMP,
    PUNYCODE_INITIAL_BIAS,
    PUNYCODE_INITIAL_N,
    decodeUtf8Codepoints,
    punycodeEncode,
    punycodeDecode,
    simdIdnaIsAscii,
    simdIdnaIsAsciiScalar,
    simdIdnaIndexOfDot,
    simdIdnaIndexOfDotScalar,
    simdIdnaToLowerAscii,
    simdIdnaToLowerAsciiScalar,
} from "./punycode.cp";

export {
    MAX_LABEL_LEN,
    MAX_DOMAIN_LEN,
    toAscii,
    toUnicode,
    isValidDomain,
    normalizeDomain,
} from "./idna.cp";

// --- highlight ---
export {
    HighlightError,
    Language,
    TokenScope,
    HighlightToken,
    Highlighter,
} from "./highlight.cp";

// --- globset ---
export {
    GLOB_MAX_TOKENS,
    GLOB_MAX_ALTERNATES,
    GLOB_MAX_BRACE_DEPTH,
    GlobError,
    GlobOptions,
    GlobMatcher,
    simdIndexOfByte,
    simdIndexOfByteScalar,
    simdIndexOfAny4,
    simdIndexOfAny4Scalar,
    simdIndexOfGlobMagic,
    simdIndexOfGlobMagicScalar,
    simdBytesEqual,
    simdBytesEqualScalar,
} from "./glob.cp";

export {
    GlobSet,
    GlobSetBuilder,
    IgnoreMatcher,
} from "./globset.cp";

// --- myersDiff ---
export {
    DEFAULT_MAX_EDIT_STEPS,
    DiffError,
    DiffKind,
    LineDiff,
    ByteDiff,
    DiffOptions,
    LineDiffResult,
    ByteDiffResult,
    scalarBytesEqual,
    simdCommonPrefixLen,
    scalarCommonPrefixLen,
    simdCommonSuffixLen,
    scalarCommonSuffixLen,
    simdFindNewline,
    scalarFindNewline,
    areLinesEqual,
    splitLines,
    diffLines,
    diffBytes,
    diffBytesWithMaxSteps,
    formatLineDiff,
} from "./myersDiff.cp";

// --- mermaid ---
export {
    MermaidError,
    FlowDirection,
    NodeShape,
    EdgeStyle,
    MsgArrow,
    ErCardinality,
    isValidIdentifier,
    FlowchartBuilder,
    SequenceBuilder,
    ErBuilder,
    StateBuilder,
} from "./mermaid.cp";

// --- dot ---
export {
    DotGraphKind,
    DotHeader,
    DotEdge,
    parseDotHeader,
    DotEdgeIter,
    validateDotBraces,
    formatDotEdge,
} from "./dot.cp";

// --- ris ---
export {
    RisTag,
    RisRecord,
    RisStats,
    RisTagIter,
    RisIter,
    isRis,
    summarizeRis,
} from "./ris.cp";

// --- gedcom ---
export {
    GedcomLine,
    GedcomHeader,
    GedcomIndividual,
    GedcomFamily,
    GedcomStats,
    GedcomLineIter,
    GedcomIndiIter,
    GedcomFamIter,
    isGedcom,
    parseGedcomLine,
    parseGedcomHeader,
    summarizeGedcom,
} from "./gedcom.cp";

// --- marc ---
export {
    MARC_LEADER_SIZE,
    MARC_DIR_ENTRY_SIZE,
    MARC_RECORD_TERMINATOR,
    MARC_FIELD_TERMINATOR,
    MARC_SUBFIELD_DELIM,
    MarcLeader,
    MarcSubfield,
    MarcSubfieldIter,
    MarcField,
    MarcFieldIter,
    MarcRecord,
    MarcRecordIter,
    isMarcRecord,
    parseMarcLeader,
} from "./marc.cp";

// --- mobi ---
export {
    PDB_HEADER_SIZE,
    PDB_RECORD_ENTRY_SIZE,
    MOBI_TYPE_SIG,
    MOBI_CREATOR_SIG,
    MOBI_MAGIC,
    EXTH_MAGIC,
    EXTH_AUTHOR,
    EXTH_PUBLISHER,
    EXTH_DESCRIPTION,
    EXTH_ISBN,
    EXTH_SUBJECT,
    EXTH_PUB_DATE,
    EXTH_ASIN,
    EXTH_UPDATED_TITLE,
    EXTH_LANGUAGE,
    MobiCompression,
    MobiEncoding,
    decodeMobiCompression,
    decodeMobiEncoding,
    PdbHeader,
    MobiHeader,
    ExthRecord,
    isMobi,
    parsePdbHeader,
    parseMobiHeader,
    ExthIter,
    findExthRecord,
} from "./mobi.cp";

// --- yenc ---
export {
    YENC_OFFSET,
    YENC_ESCAPE_OFFSET,
    YENC_ESCAPE_CHAR,
    YENC_DEFAULT_LINE_LEN,
    yencCrc32,
    yencEncode,
    yencDecode,
    YencBeginHeader,
    YencPartHeader,
    YencEndTrailer,
    parseYencBegin,
    parseYencPart,
    parseYencEnd,
} from "./yenc.cp";

// --- abc ---
export {
    AbcFraction,
    AbcHeader,
    AbcEventKind,
    AbcEvent,
    AbcStats,
    AbcEventIter,
    parseAbcHeader,
    isAbc,
    summarizeAbc,
} from "./abc.cp";

// --- smi ---
export {
    SmiTokenKind,
    SmiBracketAtom,
    SmiToken,
    SmiStats,
    SmiLexer,
    parseBracketAtom,
    analyzeSmiles,
    isValidSmiles,
} from "./smi.cp";

// --- lucene ---
export {
    LUCENE_CODEC_MAGIC,
    LUCENE_FOOTER_MAGIC,
    LUCENE_FOOTER_LENGTH,
    LUCENE_ID_LENGTH,
    LuceneVInt,
    LuceneVLong,
    LuceneString,
    LuceneCodecHeader,
    LuceneIndexHeader,
    LuceneFooter,
    readLuceneVInt,
    readLuceneVLong,
    luceneZigZagDecode32,
    luceneZigZagDecode64,
    readLuceneString,
    parseLuceneHeader,
    parseLuceneIndexHeader,
    parseLuceneFooter,
    isLuceneFile,
} from "./lucene.cp";

// --- igc ---
export {
    IgcHeader,
    IgcFix,
    IgcStats,
    isIgc,
    parseIgcHeader,
    parseIgcBRecord,
    IgcFixIter,
    summarizeIgc,
} from "./igc.cp";

// ============================================================================
// W29-Sync R3: Consolidated Text / Formatting / Encoding / String Primitives
// ============================================================================
export {
    parseByteSize,
    formatBytesIec,
    formatBytesSi,
} from "./byteSize.cp";

export {
    encodeHexLower,
    encodeHexUpper,
    decodeHexStrict,
    decodeHex,
    formatHexDumpLine,
    formatHexDump,
} from "./hexDump.cp";

export {
    TableStyle as HumanTableStyle,
    formatBytes,
    formatDurationNs,
    redactUrl,
    formatTableBorder,
    formatTableRow,
} from "./humanFmt.cp";

export {
    formatGroupedU64,
    formatGroupedI64,
    ordinalSuffix,
    formatOrdinal,
    formatRadixU64,
    formatFixedPoint,
} from "./numberFmt.cp";

export {
    HEAD_CHAR_COUNT,
    TAIL_CHAR_COUNT,
    CHAR_FREQ_COUNT,
    CharAndCount,
    CharFreq,
    NameMinifier,
} from "./charFreq.cp";

export {
    CharsetKind,
    resolveCharsetLabel,
    canonicalCharsetName,
    isUtf8Compatible,
    isAsciiSuperset,
    sniffBomCharset,
} from "./charsetLabel.cp";

export {
    CowStrKind,
    CowStr,
} from "./cowStr.cp";

export {
    MAX_INLINE_LEN,
    SmallString,
    ExternalString,
    StringInternPool,
} from "./smallStr.cp";

export {
    INLINE_MAX_LEN,
    SmolStr,
} from "./smolStr.cp";

export {
    DecodedCodepoint,
    GraphemeBreakProperty,
    GraphemeBreakState,
    GraphemeIterator,
    classifyCodepoint,
    shouldBreak,
    decodeUtf8Step,
    count,
    countScalar,
    sliceByGraphemes,
    sliceByGraphemesScalar,
    simdPrintableAsciiRun,
    simdPrintableAsciiRunScalar,
    simdIsAllPrintableAscii,
    simdIsAllPrintableAsciiScalar,
} from "./grapheme.cp";

export {
    UTF8_ACCEPT,
    UTF8_REJECT,
    REPLACEMENT_CHAR,
    DecodeStep,
    decodeByte,
    isDoneDecoding,
    Utf8Iterator,
    isValidUtf8,
    countCodepoints,
    isAscii,
    isDigit,
    isUpper,
    isLower,
    isAlphabetic,
    isAlphanumeric,
    isHex,
    isControl,
    isPrint,
    isWhitespace,
    toUpper,
    toLower,
} from "./utf8Dfa.cp";

export {
    isGlobMagic,
    hasGlobMagic,
    escapeGlob,
    unescapeGlob,
    staticBaseDir,
    isNegatedPattern,
    stripNegation,
} from "./globEscape.cp";


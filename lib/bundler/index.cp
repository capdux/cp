// Source: Absorbed from /data/dev/bun/src/bundler/ | Synced: 2026-11-20T10:24:11Z
// lib/bundler/index.cp
// Unified Public Exports for cap:bundler

export {
    BundleTarget,
    OutputFormat,
    ChunkKind,
    ChunkOrder,
    ChunkEntry,
    BundleConfig,
    BundleV2,
    INVALID_MODULE_INDEX,
    ImportKind,
    ImportRecord,
    ModuleNode,
    GraphError,
    ModuleGraph,
} from "./bundle";

export {
    TargetOs,
    TargetArch,
    TargetAbi,
    TargetParseError,
    CompileTarget,
} from "./target";

export {
    STANDALONE_HEADER_MAGIC,
    STANDALONE_TRAILER_MAGIC,
    StandaloneModuleFormat,
    StandaloneFileSide,
    StandaloneEncoding,
    StandaloneGraphError,
    StandaloneModuleEntry,
    StandaloneModuleGraph,
} from "./standalone";

export {
    MetafileImport,
    MetafileInput,
    MetafileOutputInput,
    MetafileOutput,
    MetafileError,
    Metafile,
} from "./metafile";

export {
    SplittingError,
    SplitChunk,
    AssetManifestEntry,
    AssetManifest,
    SplitResult,
    CodeSplitter,
} from "./splitting";

export {
    MAX_DEFINE_RULES,
    CallUnwrapKind,
    DefineValueKind,
    DefineFlags,
    DefineValue,
    DefineEntry,
    isValidDefineKey,
    DefineTable,
} from "./defines";

export {
    MAX_HTML_MANIFEST_FILES,
    HtmlManifestLoader,
    HtmlManifestFile,
    encodeHashEtag,
    HtmlImportManifest,
} from "./html_manifest";

export {
    MAX_OUTPUT_FILES,
    OutputFileListError,
    ChunkContentKind,
    ChunkDescriptor,
    OutputLayoutOptions,
    OutputFileCapacity,
    OutputFileSlotKind,
    OutputFileRecord,
    calculateOutputFileListCapacity,
    OutputFileListBuilder,
} from "./output_file_list";

export {
    EntryPointError,
    EntryPointNamespace,
    MacroEntryId,
    isEntryPointPath,
    encodeEntryPointPath,
    decodeEntryPointPath,
    generateFallbackEntryCode,
    generateClientEntryCode,
    generateServerEntryCode,
    generateMacroEntryId,
} from "./entry_points";

export {
    INLINE_NAME_CAP,
    MAX_SCOPE_NAMES,
    MAX_RESERVED_NAMES,
    MAX_MINIFY_SLOTS,
    DEFAULT_HEAD_ALPHABET,
    DEFAULT_TAIL_ALPHABET,
    RenamerError,
    SlotNamespace,
    InlineName,
    isJsIdentStart,
    isJsIdentContinue,
    ensureValidJsIdentifier,
    isJsReservedWord,
    isCjsReservedName,
    encodeMinifiedNameWithAlphabet,
    encodeMinifiedName,
    NumberScopeRenamer,
    MinifySymbolSlot,
    MinifySlotAssigner,
} from "./renamer";

export {
    MAX_BARREL_REQUESTED_EXPORTS,
    MAX_BARREL_REEXPORTS,
    MAX_BARREL_IMPORT_RECORDS,
    BarrelError,
    RequestedExportsKind,
    BarrelReExport,
    BarrelExportResolution,
    RequestedExportsSet,
    BarrelEligibility,
    BarrelPlanResult,
    BarrelWakeupResult,
    BarrelModule,
} from "./barrel_imports";

export {
    MAX_SCB_BOUNDARIES,
    MAX_SCB_BITSET_WORDS,
    MAX_SCB_BITSET_BITS,
    ScbError,
    UseDirective,
    ServerComponentBoundary,
    ScbBitSet,
    ServerComponentBoundaryTable,
} from "./server_components";

export {
    MAX_STATIC_ROUTE_NODES,
    MAX_STATIC_ROUTE_EDGES,
    INVALID_ROUTE_SOURCE,
    StaticRouteError,
    StaticRouteEdge,
    StaticRouteGraph,
    StaticRouteVisitor,
} from "./static_route_visitor";

export {
    ChangedFilterError,
    ChangedFilterSummary,
    AffectedEntryResult,
    ReverseImportGraph,
    parseChangedPathLines,
    freeChangedPathList,
} from "./changed_filter";

export {
    CssOrderError,
    CssOrderLoaderKind,
    CssOrderGraph,
} from "./css_order";

export {
    CSS_CHUNK_MAX_PARTS,
    CssChunkPostProcessError,
    CssChunkSourceOffset,
    CssChunkCompileResult,
    CssChunkPostProcessOptions,
    CssSourceMapEntry,
    CssChunkPostProcessResult,
    postProcessCssChunk,
} from "./css_chunk_postprocess";

export {
    MODULE_INFO_MAX_STRINGS,
    MODULE_INFO_STRING_BUF_CAP,
    MODULE_INFO_MAX_RECORDS,
    MODULE_INFO_MAX_BUFFER_IDS,
    MODULE_INFO_MAX_REQUESTED,
    MODULE_STRING_STAR_DEFAULT,
    MODULE_STRING_STAR_NAMESPACE,
    MODULE_FETCH_NONE,
    MODULE_FETCH_JAVASCRIPT,
    MODULE_FETCH_WEBASSEMBLY,
    MODULE_FETCH_JSON,
    ModuleInfoError,
    ModuleRecordKind,
    ModuleInfoFlags,
    TranspiledModuleInfo,
} from "./module_info";

export {
    CROSS_CHUNK_MAX_CHUNKS,
    CROSS_CHUNK_MAX_SYMBOLS,
    CROSS_CHUNK_MAX_USES,
    CROSS_CHUNK_MAX_EDGES,
    CrossChunkError,
    CrossChunkImportKind,
    CrossChunkNode,
    CrossChunkSymbol,
    CrossChunkImportEdge,
    CrossChunkExportEntry,
    CrossChunkResult,
    CrossChunkAnalyzer,
} from "./cross_chunk_deps";

export {
    PathTemplateError,
    PathTemplateField,
    PathSeparatorStyle,
    formatTruncatedHashHex,
    PathTemplatePlaceholder,
    PathTemplate,
    formatBundlerNaming,
} from "./path_template";

export {
    PureGlobalKind,
    PureGlobalFlags,
    lookupPureGlobalIdentifier,
    isPureGlobalIdentifier,
    isConstantFoldableGlobal,
    isPureGlobalPropertyAccess,
    isPureGlobalPropertyChain,
    isPureGlobalCallSafeForString,
    classifyGlobalExpr,
} from "./pure_globals";

export {
    HTML_SCANNER_MAX_ASSETS,
    HTML_SCANNER_MAX_SRCSET_CANDIDATES,
    HtmlScannerError,
    HtmlImportKind,
    HtmlTagKind,
    HtmlAssetRef,
    SrcsetCandidate,
    HtmlRewritePair,
    HtmlScanResult,
    isExternalHtmlUrl,
    parseSrcsetCandidates,
    scanHtmlAssets,
    rewriteHtmlAssetUrls,
} from "./html_scanner";

export {
    JS_ORDER_MAX_FILES,
    JS_ORDER_MAX_PARTS_PER_FILE,
    JS_ORDER_MAX_IMPORTS_PER_PART,
    JS_ORDER_MAX_RANGES,
    JS_ORDER_RUNTIME_SOURCE_INDEX,
    JS_ORDER_INVALID_SOURCE_INDEX,
    JS_ORDER_NAMESPACE_EXPORT_PART,
    JsOrderError,
    JsPartWrapKind,
    JsOrderImportKind,
    JsOrderImportRecord,
    JsOrderPart,
    JsOrderFile,
    JsPartRange,
    JsChunkOrderEntry,
    JsOrderResult,
    JsOrderGraph,
} from "./js_order";

export {
    HTML_CHUNK_MAX_PRELOADS,
    HtmlChunkError,
    HtmlEndTagIndices,
    HtmlChunkCompileOptions,
    HtmlChunkCompileResult,
    findHtmlEndTagIndices,
    writeEscapedAttrUrl,
    writeEscapedInlineScript,
    writeEscapedInlineStyle,
    compileHtmlChunk,
} from "./html_chunk";

export {
    CSS_IMPORT_MAX_FILES,
    CSS_IMPORT_MAX_RULES_PER_FILE,
    CSS_IMPORT_MAX_CONDITIONS,
    CSS_IMPORT_MAX_ORDER_ENTRIES,
    CSS_IMPORT_MAX_DEPTH,
    CssImportOrderError,
    CssImportCondition,
    CssConditionStack,
    CssImportRuleKind,
    CssImportRule,
    CssImportFile,
    CssImportEntryKind,
    CssImportOrderEntry,
    CssImportOrderResult,
    CssImportGraph,
} from "./css_import_order";

export {
    CHUNK_PLAN_MAX_ENTRIES,
    CHUNK_PLAN_MAX_FILES,
    CHUNK_PLAN_MAX_CHUNKS,
    CHUNK_PLAN_MAX_FILES_PER_CHUNK,
    CHUNK_PLAN_MAX_CSS_DEPS,
    CHUNK_PLAN_INVALID_INDEX,
    CHUNK_PLAN_KEY_CAP,
    ChunkPlanError,
    ChunkPlanContentKind,
    ChunkPlanLoaderKind,
    ChunkPlanEntryKind,
    ChunkPlanTarget,
    ChunkPlanTemplateKind,
    ChunkPlanOptions,
    ChunkPlanFileInput,
    ChunkPlanUniqueKey,
    PlannedChunk,
    EntryChunkMapping,
    ChunkPlanResult,
    hashCssOrderKey,
    formatChunkUniqueKey,
    ChunkPlanner,
} from "./chunk_planner";

export {
    CHUNK_RENAME_MAX_SYMBOLS,
    CHUNK_RENAME_MAX_RESERVED,
    CHUNK_RENAME_NAME_CAP,
    ChunkRenameError,
    ChunkStableRef,
    ChunkSlotCounts,
    ChunkCharFreq,
    ChunkRenameSymbolKind,
    ChunkRenameSymbolInput,
    ChunkRenamedName,
    ChunkRenameEntry,
    ChunkRenameOptions,
    ChunkRenameResult,
    ChunkSymbolRenamer,
} from "./chunk_renamer";

export {
    CHUNK_OUTPUT_MAX_PIECES,
    CHUNK_OUTPUT_MAX_REFS,
    CHUNK_DEBUG_ID_COMMENT_LEN,
    ChunkOutputError,
    ChunkPieceQueryKind,
    ChunkPieceQuery,
    ChunkPiece,
    ChunkInlineKind,
    ChunkRefTarget,
    ChunkOutputAssembleOptions,
    chunkClosingTagForKind,
    chunkExtensionForKind,
    countClosingTagOccurrences,
    escapeClosingTagsInto,
    normalizedChunkImportPathLen,
    normalizeChunkImportPath,
    formatChunkDebugIdComment,
    ChunkPieceAssembler,
} from "./chunk_output";

export {
    GRAPH_MAX_INPUT_FILES,
    GRAPH_MAX_ENTRY_POINTS,
    GRAPH_MAX_HTML_IMPORTS,
    GRAPH_MAX_PATH_LEN,
    GraphError,
    GraphLoaderKind,
    GraphScanTransition,
    GraphInputFileFlags,
    GraphInputFile,
    GraphEntryPoint,
    GraphHtmlImportPair,
    GraphScanStats,
    BundlerGraph,
} from "./graph";

export {
    PATH_SRC_MAP_BUCKETS,
    PATH_SRC_MAP_MAX_LOAD,
    PATH_SRC_INVALID_INDEX,
    PathSourceMapError,
    BundlerModulePath,
    GetOrPutSourceResult,
    PathToSourceIndexMap,
} from "./path_source_map";

// Source: Absorbed from /data/dev/bun/src/bundler/ | Synced: 2026-11-22T12:49:17Z
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
} from "./htmlManifest";

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
} from "./outputList";

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
} from "./entryPoints";

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
} from "./barrel";

export {
    MAX_SCB_BOUNDARIES,
    MAX_SCB_BITSET_WORDS,
    MAX_SCB_BITSET_BITS,
    ScbError,
    UseDirective,
    ServerComponentBoundary,
    ScbBitSet,
    ServerComponentBoundaryTable,
} from "./rsc";

export {
    MAX_STATIC_ROUTE_NODES,
    MAX_STATIC_ROUTE_EDGES,
    INVALID_ROUTE_SOURCE,
    StaticRouteError,
    StaticRouteEdge,
    StaticRouteGraph,
    StaticRouteVisitor,
} from "./staticRoute";

export {
    ChangedFilterError,
    ChangedFilterSummary,
    AffectedEntryResult,
    ReverseImportGraph,
    parseChangedPathLines,
    freeChangedPathList,
} from "./changedFilt";

export {
    CssOrderError,
    CssOrderLoaderKind,
    CssOrderGraph,
} from "./cssOrder";

export {
    CSS_CHUNK_MAX_PARTS,
    CssChunkPostProcessError,
    CssChunkSourceOffset,
    CssChunkCompileResult,
    CssChunkPostProcessOptions,
    CssSourceMapEntry,
    CssChunkPostProcessResult,
    postProcessCssChunk,
} from "./cssPost";

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
} from "./modInfo";

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
} from "./crossChunk";

export {
    PathTemplateError,
    PathTemplateField,
    PathSeparatorStyle,
    formatTruncatedHashHex,
    PathTemplatePlaceholder,
    PathTemplate,
    formatBundlerNaming,
} from "./pathTpl";

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
} from "./pureGlobals";

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
} from "./htmlScan";

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
} from "./jsOrder";

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
} from "./htmlChunk";

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
} from "./cssImpOrder";

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
} from "./chunkPlan";

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
} from "./chunkRename";

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
} from "./chunkOut";

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
} from "./pathSrcMap";

export {
    DEFERRED_BATCH_MAX_TASKS,
    DeferredBatchError,
    DeferredBatchState,
    DeferredTaskOutcome,
    DeferredPluginTask,
    DeferredDrainSummary,
    DeferredBatchCoordinator,
} from "./deferBatch";

export {
    INDEX_STR_MAP_BUCKETS,
    INDEX_STR_MAP_MAX_LOAD,
    INDEX_STR_MAP_ARENA_CAP,
    IndexStringMapError,
    IndexStringEntry,
    IndexStringMap,
} from "./idxStrMap";

export {
    OUTPUT_FILE_MAX_COLLECTION,
    OutputFileError,
    OutputFileKind,
    OutputValueKind,
    OutputLoaderKind,
    OutputBakeExtra,
    BuildOutputFile,
    BuildOutputCollection,
} from "./outputFile";

export {
    LAZY_EXPORT_MAX_PROPS,
    LAZY_EXPORT_MAX_CSS_CLASSES,
    LAZY_EXPORT_MAX_COMPOSES_EDGES,
    LAZY_EXPORT_MAX_RESOLVED_TOKENS,
    LazyExportError,
    isLazyExportJsIdentifier,
    isPromotableLazyNamedExport,
    formatLazyDefaultIdent,
    LazyExportKind,
    LazyExportPropDecision,
    LazyExportPropRecord,
    LazyExportPlanSummary,
    LazyExportPlanner,
    CssComposeTargetKind,
    CssModuleClassDef,
    CssComposeEdge,
    CssComposeGraph,
} from "./lazyExport";

export {
    JS_CHUNK_MAX_PARTS,
    JsPostprocessError,
    JsChunkOutputFormat,
    JsChunkWrapKind,
    JsChunkCommentStyle,
    JsEntryTailDecision,
    shouldGenerateEsmBytecodeModuleInfo,
    validateBytecodeConfig,
    decideEntryPointTail,
    JsLineOffsetTracker,
    JsChunkPartInput,
    JsChunkAssemblyOptions,
    JsChunkAssemblyResult,
    assembleJsChunk,
} from "./jsPostprocess";

export {
    EXPORT_SCAN_MAX_MODULES,
    EXPORT_SCAN_MAX_EXPORTS,
    EXPORT_SCAN_MAX_RECORDS,
    ExportScanError,
    ScanExportsKind,
    ScanWrapKind,
    ScanImportKind,
    ScanModuleNode,
    ScanImportEdge,
    ScanExportDecl,
    ExportScanner,
} from "./exportScan";

export {
    WRITE_PLAN_MAX_FILES,
    WRITE_PLAN_MAX_DIRS,
    WritePlanError,
    validateOutputRelPath,
    extractParentDirPosix,
    WriteChunkContentKind,
    WriteSourceMapMode,
    WriteFileRole,
    formatSourceMapUrlComment,
    WritePlannedFile,
    WritePlanStats,
    DiskWritePlanner,
} from "./writePlan";

export {
    CSS_PREP_MAX_RULES,
    CSS_PREP_MAX_CONDITIONS,
    CSS_PREP_SCRATCH_CAP,
    CssChunkPrepError,
    CssPrepRuleKind,
    CssPrepRule,
    CssPrepRuleList,
    CssPrepCondition,
    CssPrepPrefixStats,
    stripCssPrefixImportsPreservingLayers,
    wrapCssBodyWithConditions,
    formatNestedExternalImport,
    dedupCssChunkRulesReverse,
} from "./cssChunkPrep";

export {
    NS_EXPORT_MAX_ENTRIES,
    NS_EXPORT_MAX_AMBIG_REFS,
    NS_EXPORT_MAX_BINDINGS,
    NS_EXPORT_MAX_BIND_HOPS,
    NsExportError,
    NsSymbolRef,
    NsImportBinding,
    NsResolvedExport,
    NsExportExprKind,
    NsEmittedExport,
    NsExportEmitOptions,
    NsExportStats,
    NsExportPlanner,
    isSimpleJsIdent,
} from "./nsExport";

export {
    HMR_MAX_STMTS,
    HMR_MAX_IMPORTS,
    HMR_MAX_NAMED_ITEMS,
    HmrStmtsError,
    HmrImportTag,
    HmrImportRecord,
    HmrStmtKind,
    HmrStmtInput,
    HmrPrefixDeclKind,
    HmrPrefixDecl,
    HmrEsmBindingSlot,
    HmrConvertOptions,
    HmrConvertStats,
    HmrBuiltinEvent,
    parseHmrBuiltinEvent,
    hmrBuiltinEventName,
    isDirectImportMetaHotCall,
    HmrChunkStmtConverter,
} from "./hmrStmts";

export {
    RSC_PROXY_MAX_EXPORTS,
    RscProxyError,
    RscTaskKind,
    RscTargetPlatform,
    RscFrameworkConfig,
    RscProxyModuleMeta,
    rscTargetForTask,
    formatRscProdModuleKey,
    formatRscServerCallErrorMsg,
    formatClientEntryWrapper,
    RscProxyGenerator,
} from "./rscProxy";

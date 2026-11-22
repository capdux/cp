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

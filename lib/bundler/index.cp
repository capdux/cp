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

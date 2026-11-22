// Source: Absorbed from /data/dev/bun/src/resolver/ | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/resolver/index.cp — Unified Public Exports for cap:resolver
// ============================================================================

export {
    DataURL,
    DataUrlError,
} from "./dataUrl";

export {
    ModuleType,
    ExportResolveStatus,
    ExportResolveResult,
    PackageJson,
} from "./pkgJson";

export {
    TsPathMapping,
    TsConfigJson,
} from "./tsconfig";

export {
    ResolveKind,
    ResolvedKind,
    ResolveError,
    ResolveConditions,
    StatHash,
    ResolveResult,
    ModuleResolver,
} from "./resolver";

export {
    MAX_DIGEST_LEN,
    IntegrityError,
    IntegrityTag,
    Integrity,
} from "./integrity";

export {
    HostedGitError,
    GitHost,
    HostedGitInfo,
} from "./hostedGit";

export {
    MAX_SYMLINK_DEPTH,
    RealpathError,
    CachedDirEntry,
    DirInfo,
    DirCacheStats,
    DirEntryCache,
    RealpathResult,
    RealpathResolver,
} from "./dirCache";

export {
    MAX_EXTENDS_DEPTH,
    TsConfigChainError,
    TsConfigChainResult,
    TsConfigChainLoader,
    BrowserMapAction,
    BrowserLookupResult,
    BrowserMappingEntry,
    BrowserFieldMap,
    SideEffectsMode,
    SideEffectsMatcher,
    SelfReferenceMatch,
    SelfReferenceResolver,
} from "./advanced";

export {
    NODE_FALLBACK_IMPORT_PATH,
    NODE_FALLBACK_VERSION,
    NODE_FALLBACK_COUNT,
    NodeFallbackModule,
    lookupNodeFallback,
    hasNodeFallback,
    isNodeFallbackVfsPath,
    resolveNodeFallbackFromVfsPath,
    formatNodeFallbackEntryPath,
    formatNodeFallbackPackageJsonPath,
} from "./fallbacks";

export {
    PathPlatform,
    ResolverPathError,
    ResolverPathName,
    NodeJSPathName,
    PackageSpecifier,
    isRelativeModuleSpecifier,
    isBarePackageSpecifier,
    stripNodeModulesPrefix,
} from "./pathName";

export {
    IMPORT_RECORD_INVALID_SOURCE,
    IMPORT_RECORD_MAX_COUNT,
    ImportRecordError,
    ImportKind,
    ImportRecordFlags,
    ImportRecordTag,
    ImportRecordPrintMode,
    ImportSourceRange,
    ImportRecord,
    ImportRecordList,
} from "./importRecord";

export {
    STAT_HASH_SEED,
    STAT_HTTP_DATE_LEN,
    STAT_STRONG_ETAG_LEN,
    STAT_WEAK_ETAG_LEN,
    STAT_MODE_TYPE_MASK,
    STAT_MODE_REG,
    STAT_MODE_DIR,
    STAT_MODE_LNK,
    StatHashError,
    FileStatMeta,
    formatHttpDate,
    FileStatHash,
} from "./statHash";

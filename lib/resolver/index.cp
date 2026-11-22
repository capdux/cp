// Source: Absorbed from /data/dev/bun/src/resolver/ | Synced: 2026-11-20T10:22:44Z
// ============================================================================
// lib/resolver/index.cp — Unified Public Exports for cap:resolver
// ============================================================================

export {
    DataURL,
    DataUrlError,
} from "./data_url";

export {
    ModuleType,
    ExportResolveStatus,
    ExportResolveResult,
    PackageJson,
} from "./package_json";

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
} from "./hosted_git";

export {
    MAX_SYMLINK_DEPTH,
    RealpathError,
    CachedDirEntry,
    DirInfo,
    DirCacheStats,
    DirEntryCache,
    RealpathResult,
    RealpathResolver,
} from "./dir_cache";

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
} from "./node_fallbacks";

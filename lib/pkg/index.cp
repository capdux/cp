// Source: Absorbed from /data/dev/bun/src/install/install.cp | Synced: 2026-11-22T12:49:17Z
// ============================================================================
// lib/pkg/index.cp — Unified Public Exports for Industrial Package Core (cap:pkg)
// ============================================================================

export { Integrity, IntegrityTag } from "../resolver/integrity.cp";
export * from "./types.cp";
export * from "./dependency.cp";
export * from "./resolution.cp";
export * from "./npm.cp";
export * from "./lockfile.cp";
export * from "./tarball.cp";
export * from "./linker.cp";
export * from "./manager.cp";
export * from "./workspace.cp";
export * from "./registry.cp";
export * from "./lifecycle.cp";
export * from "./migration.cp";

export {
    PackageSpecifier,
    isRelativeSpecifier,
    isBareSpecifier,
    isValidPackageName,
    parsePackageSpecifier,
} from "./name.cp";

export {
    DEFAULT_NPM_REGISTRY,
    NpmrcEntry,
    NpmrcIter,
    getNpmrcValue,
    getNpmrcBool,
    resolveRegistryForScope,
    findRegistrySetting,
} from "./npmrc.cp";

export {
    ExportTargetKind,
    ExportConditionRule,
    ExportSubpathEntry,
    ResolveStatus,
    ResolveResult,
    ExportsMap,
} from "./exports.cp";

export {
    MatcherBehavior,
    PackagePattern,
    PackageMatcher,
    wildcardMatch,
} from "./matcher.cp";

export {
    SpdxTokenKind,
    SpdxToken,
    SpdxLexer,
    isValidSpdxExpression,
    SpdxIdIter,
    isCommonPermissive,
} from "./spdx.cp";

export {
    MAX_DYNAMIC_OPTIMIZER_RULES,
    PostinstallMode,
    SemverTriple,
    PostinstallPkgInfo,
    DynamicOptimizerRule,
    PostinstallOptimizerConfig,
    findNativeBinlinkReplacement,
} from "./postinstall.cp";

export {
    MAX_OVERRIDE_RULES,
    OverrideError,
    OverrideSourceKind,
    RootDepSpec,
    OverrideRule,
    normalizeOverrideKey,
    OverrideMap,
} from "./overrides.cp";

export {
    MAX_UPDATE_REQUESTS,
    UpdateRequestError,
    PackageSubcommand,
    UpdateSpecKind,
    UpdateRequest,
    UpdateRequestList,
} from "./updateReq.cp";

export {
    MAX_YARN_LOCK_PACKAGES,
    YarnPrintError,
    YarnLockDepKind,
    YarnLockDep,
    YarnLockPackage,
    mustQuoteYarnName,
    mustQuoteYarnKey,
    YarnLockPrinter,
} from "./yarnLock.cp";

import _catalog from "./catalog.cp";
pub const CatalogError = _catalog.CatalogError;
pub const CatalogRef = _catalog.CatalogRef;
pub const CatalogProtocolEntry = _catalog.CatalogEntry;
pub const StaticCatalogMap = _catalog.CatalogMap;
pub const isCatalogProtocol = _catalog.isCatalogProtocol;
pub const isValidCatalogName = _catalog.isValidCatalogName;
pub const parseCatalogRef = _catalog.parseCatalogRef;
pub const formatCatalogRef = _catalog.formatCatalogRef;

export {
    MAX_WORKSPACE_MAP_ENTRIES,
    WorkspaceMapError,
    isIgnoredWorkspacePath,
    hasIgnoredWorkspaceSegment,
    normalizeWorkspacePath,
    WorkspaceMapEntry,
    WorkspaceMap,
} from "./wsMap.cp";

export {
    WorkspaceFilterError,
    WorkspaceFilterKind,
    WorkspaceFilterPattern,
    isIgnoredFilterDir,
    normalizeFilterPath,
    matchFilterGlob,
    WorkspaceFilter,
} from "./filter.cp";

export {
    PKG_CONFIG_VERSION_CURRENT,
    PKG_ISOLATED_STORE_DIR,
    ConfigVersionError,
    LockfileMigrationSource,
    PkgLinkerStrategy,
    PkgConfigVersion,
    formatIsolatedStoreEntry,
    formatIsolatedStorePath,
} from "./configVer.cp";

export {
    WHY_MAX_PACKAGES,
    WHY_MAX_EDGES,
    WHY_MAX_DEPTH,
    WhyError,
    WhyDepType,
    specifierSpecificity,
    WhyGlobKind,
    WhyGlobPattern,
    WhyPackage,
    WhyDependent,
    compareWhyDependents,
    WhyOptions,
    WhyGraph,
} from "./why.cp";

export {
    PKG_SCRIPT_SLOT_COUNT,
    PkgScriptListError,
    PkgScriptSlotKind,
    PkgScriptFormatMode,
    PkgScriptTrustSource,
    isScriptExecutionAllowed,
    formatNodeModulesRelativeCwd,
    PkgScriptList,
} from "./scripts.cp";

export {
    TREE_PRINTER_MAX_PACKAGES,
    TREE_PRINTER_MAX_DEPS,
    TREE_PRINTER_MAX_BINS_PER_PKG,
    TREE_PRINTER_MAX_UPDATES,
    TreePrintError,
    TreePrintSectionHeader,
    TreePrintLogLevel,
    TreePrintDecisionKind,
    TreePackageBinKind,
    TreePackageEntry,
    TreeDepEdge,
    TreeUpdateRequest,
    TreePrintOptions,
    TreePrintStats,
    formatUpdatedPackageLine,
    formatInstalledPackageLine,
    InstallTreePrinter,
} from "./treePrint.cp";

export {
    BIN_SHIM_VERSION_V1,
    BIN_SHIM_VERSION_V2,
    BIN_SHIM_VERSION_V3,
    BIN_SHIM_VERSION_V4,
    BIN_SHIM_VERSION_V5,
    BIN_SHIM_VERSION_CURRENT,
    BIN_SHIM_MAX_SHEBANG_INPUT_LEN,
    BinShimError,
    BinShimExtensionKind,
    classifyBinExtension,
    BinShimFlags,
    BinShimShebang,
    DecodedBinShim,
    decodeShimUtf16LeToUtf8,
    BinLinkingShim,
} from "./binShim.cp";

export {
    PkgOptionsError,
    PkgLogLevel,
    PkgPublishAccess,
    PkgAuthType,
    PkgPublishConfig,
    PkgDoFlags,
    PkgEnableFlags,
    PkgDepOmitFlags,
    PkgInstallOptions,
} from "./options.cp";

export {
    SECURITY_SCANNER_PROTOCOL_VERSION,
    SECURITY_SCANNER_MAX_ADVISORIES,
    SECURITY_SCANNER_MAX_PATH_DEPTH,
    SecurityScannerError,
    SecurityScannerErrorCode,
    SecurityAdvisoryLevel,
    SecurityInstallDecision,
    SecurityScanPackageInput,
    SecurityAdvisory,
    SecurityScanResults,
    validateScannerProtocolVersion,
    parseWarningConfirmation,
    formatScanRequestJson,
    parseSecurityAdvisoriesJson,
} from "./scanner.cp";

export {
    PATCH_DEFAULT_DIR,
    PATCH_CACHE_HASH_TAG,
    PATCH_HEX_HASH_LEN,
    PatchSpecError,
    PatchTargetKind,
    PatchTargetArg,
    PatchCommitSpec,
    escapePatchFilenameSegment,
    formatPatchFilename,
    normalizePatchesDir,
    computePatchContentHash,
    formatPatchCacheSubpath,
    parsePatchCacheHash,
    validateUnifiedPatchText,
} from "./patchSpec.cp";

export {
    ISOLATED_STORE_MAX_ENTRIES,
    ISOLATED_STORE_MAX_DEPS_PER_ENTRY,
    ISOLATED_STORE_MAX_PEERS,
    IsolatedStoreError,
    IsolatedEntryKind,
    IsolatedPeerRef,
    IsolatedStoreEntry,
    IsolatedStoreGraph,
    formatStorePathSegment,
    formatIsolatedStoreKey,
    formatIsolatedStoreDir,
    formatIsolatedSymlinkTarget,
    formatGlobalStorePath,
    computePeerSetHash,
    computeGlobalEntryClosureHash,
} from "./isoStore.cp";

export {
    GIT_DEFAULT_ASKPASS,
    GIT_DEFAULT_SSH_COMMAND,
    GitRepoError,
    GitRepoHost,
    GitConfigFlags,
    GitNonInteractiveEnv,
    GitRepoSpec,
    isScpLikeGitPath,
    parseGitConfigFlags,
    resolveNonInteractiveGitEnv,
} from "./repository.cp";

export {
    ISOLATED_LINK_MAX_OPS,
    IsolatedSymlinkError,
    IsolatedLinkStrategy,
    IsolatedLinkPlatform,
    IsolatedDestEntryKind,
    IsolatedLinkAction,
    IsolatedLinkOp,
    IsolatedLinkPlanStats,
    stripTrailingSlash,
    reconcileIsolatedSymlink,
    computeRelativeSymlinkTarget,
    IsolatedLinkPlan,
} from "./symlink.cp";

export {
    MANIFEST_CACHE_MAX_ENTRIES,
    MANIFEST_DEFAULT_CACHE_SUBDIR,
    ManifestCacheError,
    ManifestCacheBehavior,
    ManifestCacheState,
    ManifestLookupOutcome,
    InstallCopyBackend,
    ManifestCacheRecord,
    ManifestLookupResult,
    ManifestCacheStats,
    hashPackageManifestName,
    parseInstallCopyBackend,
    formatGlobalCacheEntryDir,
    PackageManifestCache,
} from "./manifest.cp";

export {
    FOLDER_RESOLVER_MAX_ENTRIES,
    FolderResolverError,
    FolderResolverKind,
    FolderResolutionTag,
    FolderResolutionFailKind,
    FolderNormalizedLengths,
    FolderResolutionEntry,
    FolderResolverStats,
    hashFolderPath,
    stripFolderProtocolPrefix,
    shouldCheckBundledDependencies,
    normalizeFolderPackageJsonPath,
    formatWorkspaceSearchPath,
    resolveWorkspacePublishVersion,
    FolderResolverCache,
} from "./folder.cp";

export {
    WORKSPACE_PKG_CACHE_MAX_ENTRIES,
    WORKSPACE_PKG_PATH_MAX_LEN,
    WorkspacePkgCacheError,
    JsonIndentation,
    WorkspacePkgJsonOptions,
    WorkspacePkgJsonSummary,
    WorkspacePkgCacheEntry,
    WorkspacePkgCacheStats,
    isAbsoluteWorkspacePath,
    normalizeWorkspacePkgJsonPath,
    guessJsonIndentation,
    analyzeWorkspacePackageJson,
    WorkspacePkgJsonCache,
} from "./wsCache.cp";

export {
    ISOLATED_MAX_SKIP_DIRS,
    ISOLATED_MAX_DIRNAME_LEN,
    IsolatedMaterializeError,
    IsolatedLinkerMode,
    MaterializePlatform,
    MaterializeBackend,
    MaterializeErrno,
    MaterializeRecoveryAction,
    MaterializeDecision,
    MaterializeStats,
    resolveDefaultLinkerMode,
    parseLinkerModeFlag,
    defaultMaterializeBackend,
    fallbackMaterializeBackend,
    decideMaterializeRecovery,
    resolveMaterializeDestPath,
    IsolatedMaterializer,
} from "./materializer.cp";

export {
    INSTALL_PROGRESS_NAME_CAP,
    InstallProgressError,
    InstallProgressStage,
    stagePlainLabel,
    stageEmojiPrefix,
    stageBannerLabel,
    InstallProgressTracker,
} from "./progress.cp";

export {
    VERSIONED_URL_MAX_ENTRIES,
    VersionedUrlError,
    PkgSemverTriple,
    LegacyV2VersionedUrl,
    VersionedUrl,
    VersionedUrlSet,
} from "./verUrl.cp";

export {
    PNPM_LOCK_MIN_SUPPORTED_MAJOR,
    PNPM_LOCK_MAX_PATCHES,
    PNPM_LOCK_MAX_SNAPSHOTS,
    PNPM_LOCK_MAX_IMPORTERS,
    PnpmLockError,
    PnpmLockVersion,
    parsePnpmLockfileVersion,
    PnpmDepPathSuffix,
    indexOfPnpmDepPathSuffix,
    removePnpmDepPathSuffix,
    extractPnpmPatchHash,
    extractPnpmPeersSuffix,
    PnpmPackageKey,
    parsePnpmPackageKey,
    PnpmDepResolutionKind,
    PnpmImporterDepSpec,
    parsePnpmImporterVersion,
    formatPnpmCanonicalKey,
    PnpmPatchEntry,
    PnpmSnapshotRecord,
    PnpmImporterRecord,
    PnpmMigrationStats,
    PnpmMigrationIndex,
} from "./pnpmLock.cp";

export {
    PKG_JSON_MAX_DEPS_PER_GROUP,
    PKG_JSON_MAX_TRUSTED_DEPS,
    PKG_JSON_MAX_PATCHED_DEPS,
    PkgJsonEditError,
    PkgJsonDepGroup,
    pkgJsonDepGroupFieldName,
    parsePkgJsonDepGroup,
    PkgJsonVersionPin,
    detectPkgJsonVersionPin,
    formatEditedVersionSpec,
    PkgJsonEditOptions,
    PkgJsonDepEntry,
    PkgJsonPatchEntry,
    PkgJsonEditStats,
    PkgJsonEditor,
} from "./editor.cp";

export {
    PKG_CACHE_VERSION_CURRENT,
    PKG_CACHE_MAX_HOSTNAME_INLINE,
    PKG_CACHE_LINKS_SUBDIR,
    PKG_CACHE_FALLBACK_TMP_SUBDIR,
    PkgCacheDirError,
    hashCacheKey64,
    PkgCacheDirSource,
    PkgCacheEnv,
    PkgResolvedCacheDir,
    resolvePkgCacheDirPath,
    resolvePkgTempDirFallback,
    PkgCacheSemver,
    formatCachedNpmFolderName,
    formatCachedGitHubFolderName,
    formatCachedTarballFolderName,
    PkgGlobalStoreEligibility,
    PkgGlobalStoreInput,
    checkGlobalStoreEligibility,
    formatGlobalStoreLinkDir,
} from "./cacheDir.cp";

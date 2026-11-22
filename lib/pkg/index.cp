// Source: Absorbed from /data/dev/bun/src/install/install.cp | Synced: 2026-11-22T08:20:32Z
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
export * from "./registry_client.cp";
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
} from "./postinstall_optimizer.cp";

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
} from "./update_request.cp";

export {
    MAX_YARN_LOCK_PACKAGES,
    YarnPrintError,
    YarnLockDepKind,
    YarnLockDep,
    YarnLockPackage,
    mustQuoteYarnName,
    mustQuoteYarnKey,
    YarnLockPrinter,
} from "./yarn_lock_printer.cp";

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
} from "./workspace_map.cp";

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
} from "./config_version.cp";

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
} from "./script_list.cp";

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
} from "./tree_printer.cp";

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
} from "./bin_shim.cp";

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
} from "./install_options.cp";

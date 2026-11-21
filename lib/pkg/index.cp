// Source: /data/dev/bun/src/install/install.cp
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

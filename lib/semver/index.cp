// Source: Absorbed from /data/dev/bun/src/semver/semver.cp | Synced: 2026-11-21T16:40:05Z
// ============================================================================
// lib/semver/index.cp — Semver 2.0.0 Specification & Query Engine (cap:semver)
// ============================================================================

export { Version, PartialVersion, ReleaseType } from "./version";
export { Op, Comparator, ComparatorSet, Query } from "./query";
export { SemverRange, Range } from "./range";

import { Version } from "./version";
import { SemverRange } from "./range";

pub const Semver = struct {
    pub const parse = Version.parse;
    pub const parseStrict = Version.parseStrict;
    pub const parseLoose = Version.parseLoose;
    pub const parsePartial = Version.parsePartial;
    pub const valid = Version.valid;
    pub const clean = Version.clean;
    pub const compare = Version.compare;
    pub const order = Version.order;
    pub const sort = SemverRange.sort;
    pub const rsort = SemverRange.rsort;
    pub const sortStr = SemverRange.sortStr;
    pub const rsortStr = SemverRange.rsortStr;
    pub const satisfies = SemverRange.satisfies;
    pub const satisfiesWithOptions = SemverRange.satisfiesWithOptions;
    pub const satisfiesStr = SemverRange.satisfiesStr;
    pub const satisfiesStrWithOptions = SemverRange.satisfiesStrWithOptions;
    pub const validRange = SemverRange.validRange;
    pub const validRangeBuf = SemverRange.validRangeBuf;
    pub const isValidRange = SemverRange.isValidRange;
    pub const maxSatisfying = SemverRange.maxSatisfying;
    pub const maxSatisfyingWithOptions = SemverRange.maxSatisfyingWithOptions;
    pub const minSatisfying = SemverRange.minSatisfying;
    pub const minSatisfyingWithOptions = SemverRange.minSatisfyingWithOptions;
    pub const maxSatisfyingStr = SemverRange.maxSatisfyingStr;
    pub const minSatisfyingStr = SemverRange.minSatisfyingStr;
};

pub const semver = Semver;

export {
    BumpKind,
    bumpMajor,
    bumpMinor,
    bumpPatch,
    stripPrereleaseAndBuild,
    bumpPrerelease,
    bumpVersion,
} from "./bump.cp";

export {
    DependencyKind,
    WorkspaceSpecKind,
    WorkspaceSpec,
    GitSpec,
    NpmAliasSpec,
    DependencySpec,
    PackageName,
} from "./spec.cp";

export {
    MAX_COMPARATORS_PER_GROUP,
    MAX_GROUPS,
    SemverVersion,
    Op as RangeOp,
    Comparator as RangeComparator,
    ComparatorGroup,
    SemverRange as ParsedSemverRange,
} from "./rset.cp";

export {
    SemverVersion as SatisfiesVersion,
    satisfiesRange,
    satisfiesVersion,
    isValidSemverRange,
} from "./satisfies.cp";

export {
    SemverOrder,
    ParsedSemver,
    parseSemverStrict,
    parseSemverLoose,
    compareSemver,
    compareSemverStr,
    sortSemverAsc,
    sortSemverDesc,
    maxSemverIndex,
    minSemverIndex,
} from "./sort.cp";

// Source: bun/src/semver/semver.cp
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
    pub const parsePartial = Version.parsePartial;
    pub const compare = Version.compare;
    pub const order = Version.order;
    pub const satisfies = SemverRange.satisfies;
    pub const satisfiesStr = SemverRange.satisfiesStr;
    pub const maxSatisfying = SemverRange.maxSatisfying;
    pub const minSatisfying = SemverRange.minSatisfying;
    pub const maxSatisfyingStr = SemverRange.maxSatisfyingStr;
    pub const minSatisfyingStr = SemverRange.minSatisfyingStr;
};

pub const semver = Semver;

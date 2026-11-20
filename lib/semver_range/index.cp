// Source: cp/lib/semver_range/index.cp (Unified Exports for cap:semver_range)
// Pure CP industrial-grade Semver 2.0.0 parser, npm/Cargo range expression engine, and version selector.

export {
    SemverError,
    CompOp,
    RangeOptions,
    Semver,
    Comparator,
    ComparatorSet,
    RangeExpr,
} from "./types.cp";

export {
    parseSemver,
    compare,
    comparePrecedence,
    equals,
    lt,
    lte,
    gt,
    gte,
} from "./version.cp";

export {
    parseRange,
    comparatorMatches,
    comparatorSetMatches,
    rangeSatisfiesWithOptions,
} from "./range.cp";

export {
    satisfies,
    satisfiesWithOptions,
    satisfiesStr,
    satisfiesStrWithOptions,
    maxSatisfying,
    maxSatisfyingWithOptions,
    minSatisfying,
    minSatisfyingWithOptions,
    intersects,
} from "./selector.cp";

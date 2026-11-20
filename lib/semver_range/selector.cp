// Source: cp/lib/semver_range/selector.cp (Version Selection & Range Intersection Solver)
// Implements satisfies, satisfiesStr, maxSatisfying, minSatisfying, and intersects.

const std = @import("std");
import {
    SemverError,
    CompOp,
    RangeOptions,
    Semver,
    Comparator,
    ComparatorSet,
    RangeExpr,
} from "./types.cp";
import { parseSemver, compare } from "./version.cp";
import {
    parseRange,
    comparatorMatches,
    rangeSatisfiesWithOptions,
} from "./range.cp";

pub fn satisfies(version: Semver, range: RangeExpr): bool {
    return rangeSatisfiesWithOptions(range, version, RangeOptions{ include_prerelease: false });
}

pub fn satisfiesWithOptions(version: Semver, range: RangeExpr, opts: RangeOptions): bool {
    return rangeSatisfiesWithOptions(range, version, opts);
}

pub fn satisfiesStr(alloc: std.mem.Allocator, ver_str: []const u8, range_str: []const u8): bool ! SemverError {
    let v = try parseSemver(ver_str);
    let r = try parseRange(alloc, range_str);
    defer r.deinit(alloc);
    return satisfies(v, r);
}

pub fn satisfiesStrWithOptions(
    alloc: std.mem.Allocator,
    ver_str: []const u8,
    range_str: []const u8,
    opts: RangeOptions,
): bool ! SemverError {
    let v = try parseSemver(ver_str);
    let r = try parseRange(alloc, range_str);
    defer r.deinit(alloc);
    return satisfiesWithOptions(v, r, opts);
}

pub fn maxSatisfyingWithOptions(versions: []const Semver, range: RangeExpr, opts: RangeOptions): Semver? {
    let best: Semver? = null;
    let i: usize = 0;
    while (i < versions.len) {
        let v = versions[i];
        if (rangeSatisfiesWithOptions(range, v, opts)) {
            if (best == null || compare(v, best.?) > 0) {
                best = v;
            }
        }
        i += 1;
    }
    return best;
}

pub fn maxSatisfying(versions: []const Semver, range: RangeExpr): Semver? {
    return maxSatisfyingWithOptions(versions, range, RangeOptions{ include_prerelease: false });
}

pub fn minSatisfyingWithOptions(versions: []const Semver, range: RangeExpr, opts: RangeOptions): Semver? {
    let best: Semver? = null;
    let i: usize = 0;
    while (i < versions.len) {
        let v = versions[i];
        if (rangeSatisfiesWithOptions(range, v, opts)) {
            if (best == null || compare(v, best.?) < 0) {
                best = v;
            }
        }
        i += 1;
    }
    return best;
}

pub fn minSatisfying(versions: []const Semver, range: RangeExpr): Semver? {
    return minSatisfyingWithOptions(versions, range, RangeOptions{ include_prerelease: false });
}

fn checkAllComparators(set: ComparatorSet, v: Semver): bool {
    let i: usize = 0;
    while (i < set.comparators.len) {
        if (!comparatorMatches(set.comparators[i], v)) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn setsIntersect(a: ComparatorSet, b: ComparatorSet): bool {
    // 1. If either set has an Eq comparator, verify it satisfies both sets.
    let eq_ver: Semver? = null;
    let i: usize = 0;
    while (i < a.comparators.len) {
        let c = a.comparators[i];
        if (!c.is_any && c.op == CompOp.Eq) {
            if (eq_ver == null) {
                eq_ver = c.version;
            } else if (compare(eq_ver.?, c.version) != 0) {
                return false;
            }
        }
        i += 1;
    }
    let j: usize = 0;
    while (j < b.comparators.len) {
        let c = b.comparators[j];
        if (!c.is_any && c.op == CompOp.Eq) {
            if (eq_ver == null) {
                eq_ver = c.version;
            } else if (compare(eq_ver.?, c.version) != 0) {
                return false;
            }
        }
        j += 1;
    }
    if (eq_ver != null) {
        return checkAllComparators(a, eq_ver.?) && checkAllComparators(b, eq_ver.?);
    }

    // 2. Track tightest lower bound (Gt/Gte) and tightest upper bound (Lt/Lte)
    let has_lower = false;
    let lower_ver = Semver{};
    let lower_inclusive = true;

    let has_upper = false;
    let upper_ver = Semver{};
    let upper_inclusive = true;
    let upper_sentinel = false;

    let pass: usize = 0;
    while (pass < 2) {
        let comps = if (pass == 0) { a.comparators } else { b.comparators };
        let k: usize = 0;
        while (k < comps.len) {
            let c = comps[k];
            if (!c.is_any) {
                if (c.op == CompOp.Gt || c.op == CompOp.Gte) {
                    let is_inc = c.op == CompOp.Gte;
                    if (!has_lower) {
                        has_lower = true;
                        lower_ver = c.version;
                        lower_inclusive = is_inc;
                    } else {
                        let cmp = compare(c.version, lower_ver);
                        if (cmp > 0 || (cmp == 0 && !is_inc && lower_inclusive)) {
                            lower_ver = c.version;
                            lower_inclusive = is_inc;
                        }
                    }
                } else if (c.op == CompOp.Lt || c.op == CompOp.Lte) {
                    let is_inc = c.op == CompOp.Lte;
                    if (!has_upper) {
                        has_upper = true;
                        upper_ver = c.version;
                        upper_inclusive = is_inc;
                        upper_sentinel = c.is_prerelease_sentinel;
                    } else {
                        let cmp = compare(c.version, upper_ver);
                        if (cmp < 0 || (cmp == 0 && !is_inc && upper_inclusive)) {
                            upper_ver = c.version;
                            upper_inclusive = is_inc;
                            upper_sentinel = c.is_prerelease_sentinel;
                        }
                    }
                }
            }
            k += 1;
        }
        pass += 1;
    }

    if (!has_lower || !has_upper) {
        return true;
    }

    let cmp = compare(lower_ver, upper_ver);
    if (cmp > 0) {
        return false;
    }
    if (cmp == 0) {
        return lower_inclusive && upper_inclusive;
    }

    // Handle discrete gap when lower is `> X.Y.Z` (non-prerelease) and upper is `< X.Y.(Z+1)-0`
    if (!lower_inclusive && !upper_inclusive && upper_sentinel &&
        lower_ver.prerelease.len == 0 &&
        lower_ver.major == upper_ver.major &&
        lower_ver.minor == upper_ver.minor &&
        upper_ver.patch == lower_ver.patch + 1) {
        return false;
    }

    return true;
}

pub fn intersects(range_a: RangeExpr, range_b: RangeExpr): bool {
    let i: usize = 0;
    while (i < range_a.sets.len) {
        let j: usize = 0;
        while (j < range_b.sets.len) {
            if (setsIntersect(range_a.sets[i], range_b.sets[j])) {
                return true;
            }
            j += 1;
        }
        i += 1;
    }
    return false;
}

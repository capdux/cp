// Source: cp/lib/semver_range/range.cp (npm/Cargo Range Expression Parser & Evaluator)
// Supports =, <, <=, >, >=, ^, ~, x/X/* wildcards, hyphen ranges (A - B), space intersections, and || unions.

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
import { parseUintStrict, validateIdentifiers, compare } from "./version.cp";

struct PartialVer {
    has_major: bool = false,
    major: u64 = 0,
    has_minor: bool = false,
    minor: u64 = 0,
    has_patch: bool = false,
    patch: u64 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",
}

fn isWildcardToken(tok: []const u8): bool {
    return std.mem.eql(u8, tok, "*") ||
           std.mem.eql(u8, tok, "x") ||
           std.mem.eql(u8, tok, "X");
}

fn parsePartialVer(raw: []const u8): PartialVer ! SemverError {
    let s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) {
        return SemverError.InvalidComparator;
    }
    if (s[0] == 'v' || s[0] == 'V') {
        s = s[1..];
    }
    if (s.len == 0) {
        return SemverError.InvalidComparator;
    }

    let main_and_pre = s;
    let build: []const u8 = "";
    let plus_idx = std.mem.indexOfScalar(u8, s, '+');
    if (plus_idx != null) {
        let idx = plus_idx.?;
        main_and_pre = s[0..idx];
        build = s[idx + 1 ..];
        try validateIdentifiers(build, false, true);
    }

    let core = main_and_pre;
    let prerelease: []const u8 = "";
    let dash_idx = std.mem.indexOfScalar(u8, main_and_pre, '-');
    if (dash_idx != null) {
        let idx = dash_idx.?;
        core = main_and_pre[0..idx];
        prerelease = main_and_pre[idx + 1 ..];
        try validateIdentifiers(prerelease, true, false);
    }

    if (core.len == 0) {
        return SemverError.InvalidComparator;
    }

    let parts = std.mem.splitScalar(u8, core, '.');
    let p0 = parts.next();
    let p1 = parts.next();
    let p2 = parts.next();
    let p3 = parts.next();
    if (p3 != null) {
        return SemverError.InvalidComparator;
    }

    let res = PartialVer{};
    res.prerelease = prerelease;
    res.build = build;

    if (p0 == null || p0.?.len == 0) {
        return SemverError.InvalidComparator;
    }
    let s0 = p0.?;
    if (isWildcardToken(s0)) {
        res.has_major = false;
        if (p1 != null) {
            let s1 = p1.?;
            if (!isWildcardToken(s1)) {
                return SemverError.InvalidComparator;
            }
        }
        if (p2 != null) {
            let s2 = p2.?;
            if (!isWildcardToken(s2)) {
                return SemverError.InvalidComparator;
            }
        }
        if (prerelease.len > 0 || build.len > 0) {
            return SemverError.InvalidComparator;
        }
        return res;
    }

    res.has_major = true;
    res.major = try parseUintStrict(s0);

    if (p1 == null) {
        if (prerelease.len > 0 || build.len > 0) {
            return SemverError.InvalidComparator;
        }
        return res;
    }
    let s1 = p1.?;
    if (s1.len == 0) {
        return SemverError.InvalidComparator;
    }
    if (isWildcardToken(s1)) {
        res.has_minor = false;
        if (p2 != null && !isWildcardToken(p2.?)) {
            return SemverError.InvalidComparator;
        }
        if (prerelease.len > 0 || build.len > 0) {
            return SemverError.InvalidComparator;
        }
        return res;
    }

    res.has_minor = true;
    res.minor = try parseUintStrict(s1);

    if (p2 == null) {
        if (prerelease.len > 0 || build.len > 0) {
            return SemverError.InvalidComparator;
        }
        return res;
    }
    let s2 = p2.?;
    if (s2.len == 0) {
        return SemverError.InvalidComparator;
    }
    if (isWildcardToken(s2)) {
        res.has_patch = false;
        if (prerelease.len > 0 || build.len > 0) {
            return SemverError.InvalidComparator;
        }
        return res;
    }

    res.has_patch = true;
    res.patch = try parseUintStrict(s2);
    return res;
}

fn appendComp(list: *std.ArrayListUnmanaged(Comparator), alloc: std.mem.Allocator, comp: Comparator): void ! SemverError {
    list.append(alloc, comp) catch return SemverError.OutOfMemory;
}

fn expandCaret(pv: PartialVer, list: *std.ArrayListUnmanaged(Comparator), alloc: std.mem.Allocator): void ! SemverError {
    if (!pv.has_major) {
        try appendComp(list, alloc, Comparator.any());
        return;
    }
    let lower = Semver{
        major: pv.major,
        minor: if (pv.has_minor) { pv.minor } else { 0 },
        patch: if (pv.has_patch) { pv.patch } else { 0 },
        prerelease: pv.prerelease,
        build: pv.build,
    };
    try appendComp(list, alloc, Comparator{
        op: CompOp.Gte,
        version: lower,
        is_any: false,
        is_prerelease_sentinel: false,
    });

    let upper = Semver{};
    if (pv.major > 0) {
        if (pv.major == std.math.maxInt(u64)) {
            return SemverError.IntegerOverflow;
        }
        upper.major = pv.major + 1;
        upper.minor = 0;
        upper.patch = 0;
    } else if (pv.has_minor && pv.minor > 0) {
        upper.major = 0;
        upper.minor = pv.minor + 1;
        upper.patch = 0;
    } else if (pv.has_patch && pv.has_minor && pv.minor == 0) {
        upper.major = 0;
        upper.minor = 0;
        if (pv.patch == std.math.maxInt(u64)) {
            return SemverError.IntegerOverflow;
        }
        upper.patch = pv.patch + 1;
    } else if (pv.has_minor) {
        // ^0.0 or ^0.0.x
        upper.major = 0;
        upper.minor = 1;
        upper.patch = 0;
    } else {
        // ^0 or ^0.x
        upper.major = 1;
        upper.minor = 0;
        upper.patch = 0;
    }
    upper.prerelease = "0";

    try appendComp(list, alloc, Comparator{
        op: CompOp.Lt,
        version: upper,
        is_any: false,
        is_prerelease_sentinel: true,
    });
}

fn expandTilde(pv: PartialVer, list: *std.ArrayListUnmanaged(Comparator), alloc: std.mem.Allocator): void ! SemverError {
    if (!pv.has_major) {
        try appendComp(list, alloc, Comparator.any());
        return;
    }
    let lower = Semver{
        major: pv.major,
        minor: if (pv.has_minor) { pv.minor } else { 0 },
        patch: if (pv.has_patch) { pv.patch } else { 0 },
        prerelease: pv.prerelease,
        build: pv.build,
    };
    try appendComp(list, alloc, Comparator{
        op: CompOp.Gte,
        version: lower,
        is_any: false,
        is_prerelease_sentinel: false,
    });

    let upper = Semver{};
    if (!pv.has_minor) {
        if (pv.major == std.math.maxInt(u64)) {
            return SemverError.IntegerOverflow;
        }
        upper.major = pv.major + 1;
        upper.minor = 0;
        upper.patch = 0;
    } else {
        if (pv.minor == std.math.maxInt(u64)) {
            return SemverError.IntegerOverflow;
        }
        upper.major = pv.major;
        upper.minor = pv.minor + 1;
        upper.patch = 0;
    }
    upper.prerelease = "0";

    try appendComp(list, alloc, Comparator{
        op: CompOp.Lt,
        version: upper,
        is_any: false,
        is_prerelease_sentinel: true,
    });
}

fn expandPrimitiveOrWildcard(op: CompOp?, pv: PartialVer, list: *std.ArrayListUnmanaged(Comparator), alloc: std.mem.Allocator): void ! SemverError {
    // No explicit operator or '=' with partial/wildcard
    if (op == null || op.? == CompOp.Eq) {
        if (!pv.has_major) {
            try appendComp(list, alloc, Comparator.any());
            return;
        }
        if (pv.has_major && pv.has_minor && pv.has_patch) {
            try appendComp(list, alloc, Comparator{
                op: CompOp.Eq,
                version: Semver{
                    major: pv.major,
                    minor: pv.minor,
                    patch: pv.patch,
                    prerelease: pv.prerelease,
                    build: pv.build,
                },
                is_any: false,
                is_prerelease_sentinel: false,
            });
            return;
        }
        // Partial version like 1.x or 1.2.x -> >=1.2.0 <1.3.0-0
        let lower = Semver{
            major: pv.major,
            minor: if (pv.has_minor) { pv.minor } else { 0 },
            patch: 0,
            prerelease: "",
            build: "",
        };
        try appendComp(list, alloc, Comparator{
            op: CompOp.Gte,
            version: lower,
            is_any: false,
            is_prerelease_sentinel: false,
        });

        let upper = Semver{};
        if (!pv.has_minor) {
            if (pv.major == std.math.maxInt(u64)) {
                return SemverError.IntegerOverflow;
            }
            upper.major = pv.major + 1;
        } else {
            if (pv.minor == std.math.maxInt(u64)) {
                return SemverError.IntegerOverflow;
            }
            upper.major = pv.major;
            upper.minor = pv.minor + 1;
        }
        upper.prerelease = "0";
        try appendComp(list, alloc, Comparator{
            op: CompOp.Lt,
            version: upper,
            is_any: false,
            is_prerelease_sentinel: true,
        });
        return;
    }

    let cop = op.?;
    if (!pv.has_major) {
        // e.g. >=* or <*
        if (cop == CompOp.Gte || cop == CompOp.Lte) {
            try appendComp(list, alloc, Comparator.any());
        } else {
            // >* or <* is unsatisfiable
            try appendComp(list, alloc, Comparator{
                op: CompOp.Lt,
                version: Semver{ major: 0, minor: 0, patch: 0, prerelease: "0", build: "" },
                is_any: false,
                is_prerelease_sentinel: true,
            });
        }
        return;
    }

    if (pv.has_minor && pv.has_patch) {
        try appendComp(list, alloc, Comparator{
            op: cop,
            version: Semver{
                major: pv.major,
                minor: pv.minor,
                patch: pv.patch,
                prerelease: pv.prerelease,
                build: pv.build,
            },
            is_any: false,
            is_prerelease_sentinel: false,
        });
        return;
    }

    // Partial with <, <=, >, >=
    if (cop == CompOp.Gte || cop == CompOp.Lt) {
        let ver = Semver{
            major: pv.major,
            minor: if (pv.has_minor) { pv.minor } else { 0 },
            patch: 0,
            prerelease: if (cop == CompOp.Lt) { "0" } else { "" },
            build: "",
        };
        try appendComp(list, alloc, Comparator{
            op: cop,
            version: ver,
            is_any: false,
            is_prerelease_sentinel: cop == CompOp.Lt,
        });
    } else if (cop == CompOp.Gt || cop == CompOp.Lte) {
        // >1.2 -> >=1.3.0; <=1.2 -> <1.3.0-0
        let next_major = pv.major;
        let next_minor: u64 = 0;
        if (!pv.has_minor) {
            if (pv.major == std.math.maxInt(u64)) {
                return SemverError.IntegerOverflow;
            }
            next_major = pv.major + 1;
        } else {
            if (pv.minor == std.math.maxInt(u64)) {
                return SemverError.IntegerOverflow;
            }
            next_minor = pv.minor + 1;
        }
        let mapped_op = if (cop == CompOp.Gt) { CompOp.Gte } else { CompOp.Lt };
        let ver = Semver{
            major: next_major,
            minor: next_minor,
            patch: 0,
            prerelease: if (mapped_op == CompOp.Lt) { "0" } else { "" },
            build: "",
        };
        try appendComp(list, alloc, Comparator{
            op: mapped_op,
            version: ver,
            is_any: false,
            is_prerelease_sentinel: mapped_op == CompOp.Lt,
        });
    }
}

fn findHyphenSeparator(s: []const u8): usize? {
    // Look for " - " (whitespace, '-', whitespace)
    if (s.len < 3) {
        return null;
    }
    let i: usize = 1;
    while (i + 1 < s.len) {
        if (s[i] == '-') {
            let prev = s[i - 1];
            let next = s[i + 1];
            let prev_ws = prev == ' ' || prev == '\t';
            let next_ws = next == ' ' || next == '\t';
            if (prev_ws && next_ws) {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn parseHyphenRange(alloc: std.mem.Allocator, s: []const u8, sep_idx: usize): ComparatorSet ! SemverError {
    let left_raw = std.mem.trim(u8, s[0..sep_idx], " \t\r\n");
    let right_raw = std.mem.trim(u8, s[sep_idx + 1 ..], " \t\r\n");
    if (left_raw.len == 0 || right_raw.len == 0) {
        return SemverError.InvalidHyphenRange;
    }
    if (findHyphenSeparator(right_raw) != null) {
        return SemverError.InvalidHyphenRange;
    }
    // Reject extra whitespace tokens inside left or right
    if (std.mem.indexOfAny(u8, left_raw, " \t") != null ||
        std.mem.indexOfAny(u8, right_raw, " \t") != null) {
        return SemverError.InvalidHyphenRange;
    }

    let left_pv = parsePartialVer(left_raw) catch {
        return SemverError.InvalidHyphenRange;
    };
    let right_pv = parsePartialVer(right_raw) catch {
        return SemverError.InvalidHyphenRange;
    };

    let list: std.ArrayListUnmanaged(Comparator) = .empty;
    errdefer list.deinit(alloc);

    if (left_pv.has_major) {
        try appendComp(&list, alloc, Comparator{
            op: CompOp.Gte,
            version: Semver{
                major: left_pv.major,
                minor: if (left_pv.has_minor) { left_pv.minor } else { 0 },
                patch: if (left_pv.has_patch) { left_pv.patch } else { 0 },
                prerelease: left_pv.prerelease,
                build: left_pv.build,
            },
            is_any: false,
            is_prerelease_sentinel: false,
        });
    }

    if (right_pv.has_major) {
        if (right_pv.has_minor && right_pv.has_patch) {
            try appendComp(&list, alloc, Comparator{
                op: CompOp.Lte,
                version: Semver{
                    major: right_pv.major,
                    minor: right_pv.minor,
                    patch: right_pv.patch,
                    prerelease: right_pv.prerelease,
                    build: right_pv.build,
                },
                is_any: false,
                is_prerelease_sentinel: false,
            });
        } else {
            let next_major = right_pv.major;
            let next_minor: u64 = 0;
            if (!right_pv.has_minor) {
                if (right_pv.major == std.math.maxInt(u64)) {
                    return SemverError.IntegerOverflow;
                }
                next_major = right_pv.major + 1;
            } else {
                if (right_pv.minor == std.math.maxInt(u64)) {
                    return SemverError.IntegerOverflow;
                }
                next_minor = right_pv.minor + 1;
            }
            try appendComp(&list, alloc, Comparator{
                op: CompOp.Lt,
                version: Semver{
                    major: next_major,
                    minor: next_minor,
                    patch: 0,
                    prerelease: "0",
                    build: "",
                },
                is_any: false,
                is_prerelease_sentinel: true,
            });
        }
    }

    if (list.items.len == 0) {
        try appendComp(&list, alloc, Comparator.any());
    }

    let slice = list.toOwnedSlice(alloc) catch return SemverError.OutOfMemory;
    return ComparatorSet{ comparators: slice };
}

fn isOpPrefixChar(c: u8): bool {
    return c == '<' || c == '>' || c == '=' || c == '^' || c == '~';
}

fn parseComparatorSet(alloc: std.mem.Allocator, raw: []const u8): ComparatorSet ! SemverError {
    let s = std.mem.trim(u8, raw, " \t\r\n");
    if (s.len == 0) {
        return SemverError.InvalidRange;
    }

    let hyphen_idx = findHyphenSeparator(s);
    if (hyphen_idx != null) {
        return parseHyphenRange(alloc, s, hyphen_idx.?);
    }

    let list: std.ArrayListUnmanaged(Comparator) = .empty;
    errdefer list.deinit(alloc);

    let pos: usize = 0;
    while (pos < s.len) {
        // Skip whitespace
        while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t' || s[pos] == '\r' || s[pos] == '\n')) {
            pos += 1;
        }
        if (pos >= s.len) {
            break;
        }

        // Parse optional operator prefix
        let op_kind: u8 = 0; // 0=none, 1==, 2=<, 3=<=, 4=>, 5=>=, 6=^, 7=~
        if (s[pos] == '^') {
            op_kind = 6;
            pos += 1;
        } else if (s[pos] == '~') {
            op_kind = 7;
            pos += 1;
        } else if (s[pos] == '=') {
            op_kind = 1;
            pos += 1;
        } else if (s[pos] == '<') {
            if (pos + 1 < s.len && s[pos + 1] == '=') {
                op_kind = 3;
                pos += 2;
            } else {
                op_kind = 2;
                pos += 1;
            }
        } else if (s[pos] == '>') {
            if (pos + 1 < s.len && s[pos + 1] == '=') {
                op_kind = 5;
                pos += 2;
            } else {
                op_kind = 4;
                pos += 1;
            }
        }

        // Allow optional whitespace between operator and version
        if (op_kind != 0) {
            while (pos < s.len && (s[pos] == ' ' || s[pos] == '\t')) {
                pos += 1;
            }
            if (pos >= s.len || isOpPrefixChar(s[pos])) {
                return SemverError.InvalidComparator;
            }
        }

        // Read version token until whitespace
        let tok_start = pos;
        while (pos < s.len && s[pos] != ' ' && s[pos] != '\t' && s[pos] != '\r' && s[pos] != '\n') {
            pos += 1;
        }
        let ver_tok = s[tok_start..pos];
        if (ver_tok.len == 0) {
            return SemverError.InvalidComparator;
        }

        let pv = try parsePartialVer(ver_tok);
        if (op_kind == 6) {
            try expandCaret(pv, &list, alloc);
        } else if (op_kind == 7) {
            try expandTilde(pv, &list, alloc);
        } else if (op_kind == 0) {
            try expandPrimitiveOrWildcard(null, pv, &list, alloc);
        } else if (op_kind == 1) {
            try expandPrimitiveOrWildcard(CompOp.Eq, pv, &list, alloc);
        } else if (op_kind == 2) {
            try expandPrimitiveOrWildcard(CompOp.Lt, pv, &list, alloc);
        } else if (op_kind == 3) {
            try expandPrimitiveOrWildcard(CompOp.Lte, pv, &list, alloc);
        } else if (op_kind == 4) {
            try expandPrimitiveOrWildcard(CompOp.Gt, pv, &list, alloc);
        } else if (op_kind == 5) {
            try expandPrimitiveOrWildcard(CompOp.Gte, pv, &list, alloc);
        }
    }

    if (list.items.len == 0) {
        return SemverError.InvalidRange;
    }

    let slice = list.toOwnedSlice(alloc) catch return SemverError.OutOfMemory;
    return ComparatorSet{ comparators: slice };
}

pub fn parseRange(alloc: std.mem.Allocator, input: []const u8): RangeExpr ! SemverError {
    let trimmed = std.mem.trim(u8, input, " \t\r\n");
    if (trimmed.len == 0) {
        return SemverError.EmptyInput;
    }

    let sets_list: std.ArrayListUnmanaged(ComparatorSet) = .empty;
    errdefer {
        let k: usize = 0;
        while (k < sets_list.items.len) {
            sets_list.items[k].deinit(alloc);
            k += 1;
        }
        sets_list.deinit(alloc);
    }

    let iter = std.mem.splitSequence(u8, trimmed, "||");
    while (part := iter.next()) {
        let cs = try parseComparatorSet(alloc, part);
        sets_list.append(alloc, cs) catch {
            cs.deinit(alloc);
            return SemverError.OutOfMemory;
        };
    }

    if (sets_list.items.len == 0) {
        return SemverError.InvalidRange;
    }

    let sets_slice = sets_list.toOwnedSlice(alloc) catch return SemverError.OutOfMemory;
    return RangeExpr{ sets: sets_slice };
}

pub fn comparatorMatches(comp: Comparator, v: Semver): bool {
    if (comp.is_any) {
        return true;
    }
    let cmp = compare(v, comp.version);
    switch (comp.op) {
        CompOp.Eq => { return cmp == 0; },
        CompOp.Lt => { return cmp < 0; },
        CompOp.Lte => { return cmp <= 0; },
        CompOp.Gt => { return cmp > 0; },
        CompOp.Gte => { return cmp >= 0; },
    }
}

pub fn comparatorSetMatches(set: ComparatorSet, v: Semver, opts: RangeOptions): bool {
    let i: usize = 0;
    while (i < set.comparators.len) {
        if (!comparatorMatches(set.comparators[i], v)) {
            return false;
        }
        i += 1;
    }

    // Semver / npm prerelease isolation rule:
    // If target version has a prerelease tag and include_prerelease is false,
    // at least one non-sentinel comparator in the same ComparatorSet must have
    // a prerelease tag AND the exact same [major, minor, patch] tuple.
    if (v.prerelease.len > 0 && !opts.include_prerelease) {
        let allowed = false;
        let j: usize = 0;
        while (j < set.comparators.len) {
            let c = set.comparators[j];
            if (!c.is_any && !c.is_prerelease_sentinel && c.version.prerelease.len > 0) {
                if (c.version.major == v.major &&
                    c.version.minor == v.minor &&
                    c.version.patch == v.patch) {
                    allowed = true;
                    break;
                }
            }
            j += 1;
        }
        if (!allowed) {
            return false;
        }
    }

    return true;
}

pub fn rangeSatisfiesWithOptions(range: RangeExpr, v: Semver, opts: RangeOptions): bool {
    let i: usize = 0;
    while (i < range.sets.len) {
        if (comparatorSetMatches(range.sets[i], v, opts)) {
            return true;
        }
        i += 1;
    }
    return false;
}

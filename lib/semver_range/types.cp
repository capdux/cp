// Source: cp/lib/semver_range/types.cp (Semver 2.0 & Range Expression Core Types)
// Core error taxonomy, Semver struct, CompOp, Comparator, ComparatorSet, RangeExpr, and RangeOptions.

const std = @import("std");

pub err SemverError {
    EmptyInput,
    InvalidVersion,
    LeadingZero,
    IntegerOverflow,
    InvalidPrerelease,
    InvalidBuildMetadata,
    InvalidRange,
    InvalidComparator,
    InvalidHyphenRange,
    OutOfMemory,
}

pub enum CompOp {
    Eq,
    Lt,
    Lte,
    Gt,
    Gte,
}

pub struct RangeOptions {
    include_prerelease: bool = false,
}

pub struct Semver {
    major: u64 = 0,
    minor: u64 = 0,
    patch: u64 = 0,
    prerelease: []const u8 = "",
    build: []const u8 = "",

    pub static fn init(major: u64, minor: u64, patch: u64): Semver {
        return Semver{
            major: major,
            minor: minor,
            patch: patch,
            prerelease: "",
            build: "",
        };
    }

    pub fn hasPrerelease(*_): bool {
        return _.prerelease.len > 0;
    }

    pub fn hasBuild(*_): bool {
        return _.build.len > 0;
    }

    pub fn format(*_, alloc: std.mem.Allocator): ([]u8) ! SemverError {
        let out: std.ArrayListUnmanaged(u8) = .empty;
        errdefer out.deinit(alloc);

        let writer = out.writer(alloc);
        writer.print("{d}.{d}.{d}", .{ _.major, _.minor, _.patch }) catch return SemverError.OutOfMemory;
        if (_.prerelease.len > 0) {
            writer.print("-{s}", .{_.prerelease}) catch return SemverError.OutOfMemory;
        }
        if (_.build.len > 0) {
            writer.print("+{s}", .{_.build}) catch return SemverError.OutOfMemory;
        }
        return out.toOwnedSlice(alloc) catch return SemverError.OutOfMemory;
    }
}

pub struct Comparator {
    op: CompOp = CompOp.Eq,
    version: Semver = Semver{},
    is_any: bool = false,
    is_prerelease_sentinel: bool = false,

    pub static fn any(): Comparator {
        return Comparator{
            op: CompOp.Gte,
            version: Semver{},
            is_any: true,
            is_prerelease_sentinel: false,
        };
    }
}

pub struct ComparatorSet {
    comparators: []Comparator = &.{},

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (_.comparators.len > 0) {
            alloc.free(_.comparators);
            _.comparators = &.{};
        }
    }
}

pub struct RangeExpr {
    sets: []ComparatorSet = &.{},

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.sets.len) {
            _.sets[i].deinit(alloc);
            i += 1;
        }
        if (_.sets.len > 0) {
            alloc.free(_.sets);
            _.sets = &.{};
        }
    }
}

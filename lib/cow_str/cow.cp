// Source: Absorbed from /data/dev/bun/src/shell/EnvStr.cp | Synced: 2026-11-21T12:54:25Z
//! Copy-on-Write / Reference-Counted-or-Borrowed string (`CowStr`) with
//! deterministic `~init()` RAII cleanup and O(1) atomic `clone()`.

import std from "std";

export enum CowStrKind: u8 {
    empty = 0,
    borrowed = 1,
    refcounted = 2,
}

struct RefCountedBuf {
    allocator: std.mem.Allocator,
    refcount: std.atomic.Value(usize),
    data: []u8,

    pub static fn create(allocator: std.mem.Allocator, src: []const u8): (*RefCountedBuf!)! {
        const duped = allocator.dupe(u8, src)?;
        errdefer allocator.free(duped);
        let box = allocator.create(RefCountedBuf)?;
        unsafe {
            box.* = RefCountedBuf{
                allocator: allocator,
                refcount: std.atomic.Value(usize).init(1),
                data: duped,
            };
        }
        return box;
    }

    pub fn retain(*_) {
        * = _.refcount.fetchAdd(1, .monotonic);
    }

    pub fn releaseRef(*_): bool {
        const prev = _.refcount.fetchSub(1, .acq_rel);
        if (prev == 1) {
            const alloc = _.allocator;
            alloc.free(_.data);
            alloc.destroy(_);
            return true;
        }
        return false;
    }
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// String that is either empty, a zero-copy borrowed slice, or an atomically
/// reference-counted heap string freed automatically by `~init()`.
export struct CowStr {
    kind: CowStrKind = .empty,
    borrowed: []const u8 = "",
    rc: (*RefCountedBuf!)? = null,

    pub static fn initEmpty(): CowStr {
        return CowStr{ kind: .empty, borrowed: "", rc: null };
    }

    pub static fn initBorrowed(s: []const u8): CowStr {
        if (s.len == 0) return CowStr.initEmpty();
        return CowStr{ kind: .borrowed, borrowed: s, rc: null };
    }

    pub static fn dupeOwned(allocator: std.mem.Allocator, s: []const u8): CowStr! {
        if (s.len == 0) return CowStr.initEmpty();
        const box = RefCountedBuf.create(allocator, s)?;
        return CowStr{ kind: .refcounted, borrowed: "", rc: box };
    }

    ~init() {
        if (_.kind == .refcounted) {
            if (box := _.rc) {
                unsafe {
                    * = box.releaseRef();
                }
                _.rc = null;
                _.kind = .empty;
            }
        }
    }

    /// O(1) clone: increments atomic refcount for owned strings, zero-cost copy for borrowed/empty.
    pub fn clone(): CowStr {
        if (_.kind == .refcounted) {
            if (box := _.rc) {
                unsafe {
                    box.retain();
                }
                return CowStr{ kind: .refcounted, borrowed: "", rc: box };
            }
        }
        return CowStr{ kind: _.kind, borrowed: _.borrowed, rc: null };
    }

    pub fn slice(): []const u8 {
        if (_.kind == .borrowed) return _.borrowed;
        if (_.kind == .refcounted) {
            if (box := _.rc) {
                return unsafe { box.data };
            }
        }
        return "";
    }

    pub fn len(): usize {
        return _.slice().len;
    }

    pub fn isEmpty(): bool {
        return _.len() == 0;
    }

    pub fn isOwned(): bool {
        return _.kind == .refcounted;
    }

    pub fn isBorrowed(): bool {
        return _.kind == .borrowed;
    }

    pub fn refCount(): usize {
        if (_.kind == .refcounted) {
            if (box := _.rc) {
                return unsafe { box.refcount.load(.acquire) };
            }
        }
        return 0;
    }

    /// Amortized memory cost (length divided by refcount for shared strings).
    pub fn memoryCost(): usize {
        const n = _.len();
        if (n == 0) return 0;
        const rc = _.refCount();
        if (rc == 0) return n;
        return n / rc;
    }

    pub fn eqlStr(other: []const u8): bool {
        return eqlBytes(_.slice(), other);
    }

    pub fn eql(other: CowStr): bool {
        return eqlBytes(_.slice(), other.slice());
    }
}

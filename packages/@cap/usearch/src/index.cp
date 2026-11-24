// @cap/usearch — USearch (v2.26.6) HNSW Vector Search Engine for CP
// License: Apache-2.0 (Upstream: unum-cloud/usearch)

import std from "std";

pub const USEARCH_METRIC_UNKNOWN: u32 = 0;
pub const USEARCH_METRIC_COS: u32 = 1;
pub const USEARCH_METRIC_IP: u32 = 2;
pub const USEARCH_METRIC_L2SQ: u32 = 3;

pub const USEARCH_SCALAR_UNKNOWN: u32 = 0;
pub const USEARCH_SCALAR_F32: u32 = 1;
pub const USEARCH_SCALAR_F64: u32 = 2;
pub const USEARCH_SCALAR_F16: u32 = 3;
pub const USEARCH_SCALAR_I8: u32 = 4;
pub const USEARCH_SCALAR_B1: u32 = 5;

pub @c struct UsearchInitOptions {
    metric_kind: u32 = USEARCH_METRIC_COS;
    metric: *!? = null;
    quantization: u32 = USEARCH_SCALAR_F32;
    dimensions: usize = 0;
    connectivity: usize = 0;
    expansion_add: usize = 0;
    expansion_search: usize = 0;
    multi: bool = false;
}

pub @c {
    fn usearch_version(): [*:0]const u8;
    fn usearch_init(options: *UsearchInitOptions!, err: *([*:0]const u8?)!): *!?;
    fn usearch_free(index: *!?, err: *([*:0]const u8?)!);
    fn usearch_reserve(index: *!?, capacity: usize, err: *([*:0]const u8?)!);
    fn usearch_add(index: *!?, key: u64, vector: *const anyopaque!, vector_kind: u32, err: *([*:0]const u8?)!);
    fn usearch_search(
        index: *!?,
        query_vector: *const anyopaque!,
        query_kind: u32,
        count: usize,
        keys: [*]u64!,
        distances: [*]f32!,
        err: *([*:0]const u8?)!,
    ): usize;
    fn usearch_size(index: *!?, err: *([*:0]const u8?)!): usize;
    fn usearch_dimensions(index: *!?, err: *([*:0]const u8?)!): usize;
    fn usearch_contains(index: *!?, key: u64, err: *([*:0]const u8?)!): bool;
    fn usearch_remove(index: *!?, key: u64, err: *([*:0]const u8?)!): usize;
}

pub err UsearchError {
    InvalidDimensions,
    InitFailed,
    ReserveFailed,
    AddFailed,
    SearchFailed,
    RemoveFailed,
    IndexClosed,
}

/// Returns the linked USearch library version string.
pub fn versionString(): string {
    unsafe {
        return std.mem.span(usearch_version());
    }
}

/// Owned zero-heap-allocation RAII wrapper around a USearch HNSW dense vector index.
/// Automatically releases native resources on scope exit via `~init()`.
pub struct VectorIndex {
    handle: *!? = null,
    dims: usize = 0,

    pub static fn initWithMetric(dims: usize, metric_kind: u32): VectorIndex! {
        if (dims == 0) return UsearchError.InvalidDimensions;
        let opts = UsearchInitOptions{
            .metric_kind = metric_kind,
            .metric = null,
            .quantization = USEARCH_SCALAR_F32,
            .dimensions = dims,
            .connectivity = 0,
            .expansion_add = 0,
            .expansion_search = 0,
            .multi = false,
        };
        let err: [*:0]const u8? = null;
        let idx: *!? = null;
        unsafe {
            idx = usearch_init(&opts, &err);
        }
        if (err != null or idx == null) return UsearchError.InitFailed;
        return VectorIndex{ .handle = idx, .dims = dims };
    }

    pub static fn initCosine(dims: usize): VectorIndex! {
        return VectorIndex.initWithMetric(dims, USEARCH_METRIC_COS);
    }

    pub static fn initL2(dims: usize): VectorIndex! {
        return VectorIndex.initWithMetric(dims, USEARCH_METRIC_L2SQ);
    }

    pub fn reserve(cap: usize): void! {
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            unsafe {
                usearch_reserve(h, cap, &err);
            }
            if (err != null) return UsearchError.ReserveFailed;
            return;
        }
        return UsearchError.IndexClosed;
    }

    pub fn addF32(key: u64, vec: []const f32): void! {
        if (vec.len != _.dims) return UsearchError.InvalidDimensions;
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            unsafe {
                usearch_add(h, key, @ptrCast(vec.ptr), USEARCH_SCALAR_F32, &err);
            }
            if (err != null) return UsearchError.AddFailed;
            return;
        }
        return UsearchError.IndexClosed;
    }

    pub fn searchF32(query: []const f32, k: usize, out_keys: []u64, out_dists: []f32): usize! {
        if (query.len != _.dims) return UsearchError.InvalidDimensions;
        if (h := _.handle) {
            let limit = k;
            if (out_keys.len < limit) limit = out_keys.len;
            if (out_dists.len < limit) limit = out_dists.len;
            if (limit == 0) return 0;

            let err: [*:0]const u8? = null;
            let found: usize = 0;
            unsafe {
                found = usearch_search(
                    h,
                    @ptrCast(query.ptr),
                    USEARCH_SCALAR_F32,
                    limit,
                    out_keys.ptr,
                    out_dists.ptr,
                    &err,
                );
            }
            if (err != null) return UsearchError.SearchFailed;
            return found;
        }
        return UsearchError.IndexClosed;
    }

    pub fn size(): usize {
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            let n: usize = 0;
            unsafe {
                n = usearch_size(h, &err);
            }
            if (err != null) return 0;
            return n;
        }
        return 0;
    }

    pub fn dimensions(): usize {
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            let d: usize = 0;
            unsafe {
                d = usearch_dimensions(h, &err);
            }
            if (err != null) return 0;
            return d;
        }
        return 0;
    }

    pub fn contains(key: u64): bool {
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            let ok = false;
            unsafe {
                ok = usearch_contains(h, key, &err);
            }
            if (err != null) return false;
            return ok;
        }
        return false;
    }

    pub fn remove(key: u64): usize! {
        if (h := _.handle) {
            let err: [*:0]const u8? = null;
            let dropped: usize = 0;
            unsafe {
                dropped = usearch_remove(h, key, &err);
            }
            if (err != null) return UsearchError.RemoveFailed;
            return dropped;
        }
        return UsearchError.IndexClosed;
    }

    ~init() {
        if (h := _.handle) {
            _.handle = null;
            let err: [*:0]const u8? = null;
            unsafe {
                usearch_free(h, &err);
            }
        }
    }
}

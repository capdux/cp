// Type definitions for CP

import {  } from "std";
export class UsearchInitOptions {
    metric_kind: u32,
    metric: *auto!?,
    quantization: u32,
    dimensions: usize,
    connectivity: usize,
    expansion_add: usize,
    expansion_search: usize,
    multi: bool,
}

export fn usearch_version(): [*:0]&u8!;
export fn usearch_init(options: *UsearchInitOptions!, err: *[*:0]&u8!?!): *auto!?;
export fn usearch_free(index: *auto!?, err: *[*:0]&u8!?!);
export fn usearch_reserve(index: *auto!?, capacity: usize, err: *[*:0]&u8!?!);
export fn usearch_add(index: *auto!?, key: u64, vector: *&anyopaque!, vector_kind: u32, err: *[*:0]&u8!?!);
export fn usearch_search(index: *auto!?, query_vector: *&anyopaque!, query_kind: u32, count: usize, keys: [*]u64!, distances: [*]f32!, err: *[*:0]&u8!?!): usize;
export fn usearch_size(index: *auto!?, err: *[*:0]&u8!?!): usize;
export fn usearch_dimensions(index: *auto!?, err: *[*:0]&u8!?!): usize;
export fn usearch_contains(index: *auto!?, key: u64, err: *[*:0]&u8!?!): bool;
export fn usearch_remove(index: *auto!?, key: u64, err: *[*:0]&u8!?!): usize;
export error UsearchError { InvalidDimensions, InitFailed, ReserveFailed, AddFailed, SearchFailed, RemoveFailed, IndexClosed }

export fn versionString(): string;
export class VectorIndex {
    handle: *auto!?,
    dims: usize,
    fn initWithMetric(dims: usize, metric_kind: u32): !VectorIndex;
    fn initCosine(dims: usize): !VectorIndex;
    fn initL2(dims: usize): !VectorIndex;
    fn reserve(cap: usize): !void;
    fn addF32(key: u64, vec: &[]f32): !void;
    fn searchF32(query: &[]f32, k: usize, out_keys: []u64, out_dists: []f32): !usize;
    fn size(): usize;
    fn dimensions(): usize;
    fn contains(key: u64): bool;
    fn remove(key: u64): !usize;
    fn __cp_deinit(_: &mut VectorIndex);
}


# @cap/usearch@2.26.6 (Agent Quickref)

> USearch (v2.26.6) high-performance HNSW vector search engine for CP AI Agents | License: Apache-2.0
> Import: `import { ... } from "@cap/usearch";` | Full types: `src/index.d.cp`

## Functions
- `export fn usearch_version(): [*:0]&u8!;`
- `export fn usearch_init(options: *UsearchInitOptions!, err: *[*:0]&u8!?!): *auto!?;`
- `export fn usearch_free(index: *auto!?, err: *[*:0]&u8!?!);`
- `export fn usearch_reserve(index: *auto!?, capacity: usize, err: *[*:0]&u8!?!);`
- `export fn usearch_add(index: *auto!?, key: u64, vector: *&anyopaque!, vector_kind: u32, err: *[*:0]&u8!?!);`
- `export fn usearch_search(index: *auto!?, query_vector: *&anyopaque!, query_kind: u32, count: usize, keys: [*]u64!, distances: [*]f32!, err: *[*:0]&u8!?!): usize;`
- `export fn usearch_size(index: *auto!?, err: *[*:0]&u8!?!): usize;`
- `export fn usearch_dimensions(index: *auto!?, err: *[*:0]&u8!?!): usize;`
- `export fn usearch_contains(index: *auto!?, key: u64, err: *[*:0]&u8!?!): bool;`
- `export fn usearch_remove(index: *auto!?, key: u64, err: *[*:0]&u8!?!): usize;`
- `export fn versionString(): string;`

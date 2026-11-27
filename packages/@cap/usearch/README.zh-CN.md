[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# @cap/usearch

> 版本：`2.26.6` | 许可证：`Apache-2.0` | 类型声明：[src/index.d.cp](./src/index.d.cp)

USearch (v2.26.6) high-performance HNSW vector search engine for CP AI Agents

## 安装与导入

```bash
cap add @cap/usearch
```

```cp
import { usearch_version } from "@cap/usearch";
```

## 核心导出 API

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

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`c++` (`usearch@2.26.6`)，许可证：Apache-2.0

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/disjoint-set`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:disjoint-sets` ([https://github.com/tov/disjoint-sets-rs](https://github.com/tov/disjoint-sets-rs))（原库名 `disjoint-sets` → CP 包名 `@cap/disjoint-set`）

Zero-heap Disjoint Set Union (Union-Find) data structure in pure CP, featuring path halving and union by size. Ideal for connected components, Kruskal MST, equivalence classes, and cycle detection in undirected graphs.

零堆分配纯 CP 并查集（Disjoint Set Union / Union-Find），内置路径减半压缩与按秩/大小合并，适用于无向图连通分量、Kruskal 最小生成树、等价类划分与环路检测。

## Installation

```bash
cap add @cap/disjoint-set
```

## API

| Method | Description |
| --- | --- |
| `DisjointSet.init(n)` | Initialize `n` singleton sets (clamped to `MAX_ELEMENTS = 256`) |
| `len()` | Total number of elements |
| `componentCount()` | Current number of disjoint sets |
| `find(x)` | Find representative root with path halving (`-1` if out of bounds) |
| `findReadOnly(x)` | Non-mutating root lookup (`-1` if out of bounds) |
| `unite(a, b)` | Merge sets containing `a` and `b`; returns `true` if merged |
| `connected(a, b)` | Check if `a` and `b` belong to the same set |
| `setSize(x)` | Number of elements in `x`'s set (`0` if out of bounds) |
| `reset(n)` | Reset structure in-place for `n` elements |

## License

MIT

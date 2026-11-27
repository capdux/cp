[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rangemap

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rangemap` ([https://github.com/jeffparsons/rangemap](https://github.com/jeffparsons/rangemap))（原库名 `rangemap` → CP 包名 `rangemap`）

零堆分配的 CP 有序不重叠半开区间集合 `[start, end)` — 洁净室 MIT 实现，对标 Rust `rangemap` / `range-set` 与 npm `merge-ranges`，自动合并相邻/重叠区间，支持区间差集与中间劈裂、$O(\log N)$ 二分点查/区间覆盖/重叠检测与覆盖总长统计。

Zero-allocation sorted disjoint half-open interval set `[start, end)` for CP — clean-room MIT parity with Rust `rangemap`/`range-set` and npm `merge-ranges`.

## 安装 / Install

```bash
cap add rangemap
```

## API 参考 / API Reference

- **`Range`**：`{ start: i64, end: i64 }`
- **`RangeSet`**（容量 `MAX_RANGES = 128`）：
  - `init()`, `len()`, `isEmpty()`, `clear(*_)`, `get(idx) -> Range`
  - `insert(*_, start, end) -> bool`（自动合并相邻 `a.end == b.start` 与重叠区间）
  - `remove(*_, start, end) -> bool`（截断、整段删除与中间劈裂）
  - `contains(point)`, `containsRange(start, end)`, `overlaps(start, end)`, `totalSpan()`

## 使用示例 / Usage

```cp
import { RangeSet } from "rangemap";

let rs = RangeSet.init();
* = rs.insert(0, 10);
* = rs.insert(10, 20); // coalesced into [0, 20)
* = rs.remove(5, 12);  // split into [0, 5) and [12, 20)
const span = rs.totalSpan(); // 13
```

## License

MIT

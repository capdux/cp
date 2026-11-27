# @cap/count-min

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:count-min-sketch` ([https://github.com/amansarda/count-min-sketch](https://github.com/amansarda/count-min-sketch))（原库名 `count-min-sketch` → CP 包名 `@cap/count-min`）

Zero-allocation Count-Min Sketch streaming frequency estimator and heavy-hitter detector for CP — clean-room MIT parity with Rust `count-min-sketch` and Redis `CMS.INCRBY`.

纯 CP 零堆分配流式频次估算引擎（4 行 × 256 列饱和计数器），严格保证永不低估真实频次，支持分布式节点合并与热点 Key 检测。

## Install / 安装

```bash
cap add @cap/count-min
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `CountMinSketch` | `init()`, `clear(*_)`, `width() -> usize`, `depth() -> usize`, `totalCount() -> u64` | 4×256 fixed-capacity Count-Min Sketch |
| `add` / `increment` / `addU64` | `(*_, item/val, count)` | Saturating counter updates |
| `estimate` / `estimateU64` | `(item/val) -> u32` | Minimum across 4 rows (never underestimates true count) |
| `merge` | `(*_, other: CountMinSketch)` | Element-wise saturating merge for distributed aggregation |
| `isHeavyHitter` | `(item: string, threshold_ratio: f64) -> bool` | Detect if estimated share meets or exceeds `threshold_ratio` |

## Usage / 使用示例

```cp
import { CountMinSketch } from "@cap/count-min";

let cms = CountMinSketch.init();
cms.add("/api/login", 500 as u32);
cms.increment("/api/health");
const est = cms.estimate("/api/login");           // >= 500
const hot = cms.isHeavyHitter("/api/login", 0.5); // true
```

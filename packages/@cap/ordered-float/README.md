# @cap/ordered-float

零堆分配的 CP IEEE 754 全序浮点比较、`NotNanF64` 安全包装器与含 `NaN`/`±Inf` 确定性原地排序引擎 — 洁净室 MIT 实现，对标 Rust crates.io #292 `ordered-float`（5.71 亿+ 总下载）。

Zero-allocation IEEE 754 total-order float comparison, `NotNanF64` checked wrapper, and NaN-safe in-place sorting & binary search for CP — clean-room MIT parity with Rust `ordered-float` (571M+ downloads).

## 安装 / Install

```bash
cap add @cap/ordered-float
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `isNanF64` / `isInfF64` / `isFiniteF64` | 浮点分类判断 |
| `orderedCmpF64(a, b) -> i32` | 严格全序比较（`-Inf < finite < +Inf < NaN`，两 `NaN` 相等） |
| `orderedEqF64(a, b) -> bool` | 全序相等性（`NaN == NaN` 为 `true`，`-0.0 == +0.0` 为 `true`） |
| `orderedMinF64` / `orderedMaxF64` / `clampF64` | 全序最值与区间夹紧 |
| `NotNanF64` | 非 NaN 安全包装器：`new(v) -> NotNanF64?`, `value()`, `cmp(other)` |
| `sortF64(slice)` | 零堆原地升序排序（所有 `NaN` 稳定归至末尾，绝不死循环） |
| `isSortedF64(slice)` / `binarySearchF64(slice, target)` | 全序有序性校验与二分查找 |

## 使用示例 / Usage

```cp
import { orderedCmpF64, orderedEqF64, NotNanF64, sortF64, binarySearchF64 } from "@cap/ordered-float";

let nums: [4]f64 = [3.5, 0.0 / 0.0, -1.0, 2.0];
sortF64(nums[0..4]);
// => [-1.0, 2.0, 3.5, NaN]
const idx = binarySearchF64(nums[0..4], 2.0).?; // 1
```

## License

MIT

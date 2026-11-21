# `cap:hilbert` — 2D Hilbert 空间填充曲线与 2D/3D Morton Z-Order 空间索引库

零堆分配（Zero-Alloc）的纯 CP 空间填充曲线与多维空间索引库，完整支持 1..32 阶 64-bit 2D Hilbert 曲线与 O(1) 无分支 SWAR 位交错 2D/3D Morton（Z-Order）曲线，适用于地理空间索引、数据库多维聚簇键、四叉树/八叉树加速与缓存局部性优化。

## 核心特性

- **零堆分配**：纯整数位运算与栈值返回，无任何动态内存分配。
- **满 64-bit 2D Hilbert 曲线**：支持 `order = 1..32`（网格 `2^1 × 2^1` 至 `2^32 × 2^32`），内部采用 `u64` 子象限掩码旋转，彻底消除 `order == 32` 下的移位与减法溢出。
- **严格拓扑保距**：任意相邻索引 `i` 与 `i + 1` 解码出的 2D 格点满足曼哈顿距离 `manhattanDist2d == 1`。
- **O(1) 无分支 SWAR Morton 编码**：2D 支持完整 32-bit 坐标（64-bit 交错键）；3D 支持完整 21-bit 坐标 `0..0x1FFFFF`（63-bit 交错键）。

## 导出 API

### 协议常量

| 常量 | 类型 | 值 | 说明 |
| --- | --- | --- | --- |
| `HILBERT_MIN_ORDER` | `u8` | `1` | 最小 2D Hilbert 阶数（2×2 网格） |
| `HILBERT_MAX_ORDER` | `u8` | `32` | 最大 2D Hilbert 阶数（`2^32 × 2^32` 网格，满 64-bit 索引） |
| `MORTON3D_MAX_COORD` | `u32` | `0x1FFFFF` | 3D Morton 单轴最大坐标（21-bit） |

### 数据模型

```cp
pub struct Point2D {
    x: u32,
    y: u32,
}

pub struct Point3D {
    x: u32,
    y: u32,
    z: u32,
}
```

### 2D Hilbert 曲线

- `hilbert2dMaxIndex(order: u8): u64?`
  返回 `order` 阶 Hilbert 曲线的最大合法索引 `2^(2*order) - 1`（`order == 32` 返回 `0xFFFFFFFFFFFFFFFF`）；`order` 不在 `1..32` 返回 `null`。
- `hilbert2dEncode(order: u8, x: u32, y: u32): u64?`
  将 2D 格点 `(x, y)` 编码为 64-bit Hilbert 索引；越界阶数或越界坐标返回 `null`。
- `hilbert2dDecode(order: u8, index: u64): Point2D?`
  将 64-bit Hilbert 索引解码为 `Point2D`；越界阶数或越界索引返回 `null`。

### 2D / 3D Morton (Z-Order) 空间索引

- `morton2dEncode(x: u32, y: u32): u64`
  将 32-bit `(x, y)` 按 SWAR 位交错编码为 64-bit Z-Order 键（偶数位存 `x`，奇数位存 `y`）。
- `morton2dDecode(z: u64): Point2D`
  将 64-bit Z-Order 键解交错还原为 `Point2D`。
- `morton3dEncode(x: u32, y: u32, z: u32): u64?`
  将 21-bit `(x, y, z)` 编码为 63-bit Z-Order 键；任意分量 `> 0x1FFFFF` 返回 `null`。
- `morton3dDecode(code: u64): Point3D`
  将 63-bit 3D Z-Order 键解交错还原为 `Point3D`。

### 邻接性辅助

- `manhattanDist2d(a: Point2D, b: Point2D): u64`
  计算两点间防溢出的 L1 曼哈顿距离 `|a.x - b.x| + |a.y - b.y|`。

## 使用示例

```cp
import {
    Point2D,
    hilbert2dEncode,
    hilbert2dDecode,
    morton2dEncode,
    morton2dDecode,
    morton3dEncode,
    morton3dDecode,
    manhattanDist2d,
} from "cap:hilbert";

fn example() {
    if (idx := hilbert2dEncode(16, 1234, 5678)) {
        if (p := hilbert2dDecode(16, idx)) {
            // p.x == 1234 && p.y == 5678
            if (next := hilbert2dDecode(16, idx + 1)) {
                // manhattanDist2d(p, next) == 1
                * = manhattanDist2d(p, next);
            }
        }
    }

    const z2 = morton2dEncode(42, 99);
    const p2 = morton2dDecode(z2);
    * = p2;

    if (z3 := morton3dEncode(100, 200, 300)) {
        const p3 = morton3dDecode(z3);
        * = p3;
    }
}
```

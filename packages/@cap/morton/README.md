# @cap/morton

Zero-allocation 2D/3D Z-Order (Morton) bit-interleaving codec and 2D Hilbert space-filling curve mapper for CP — clean-room MIT parity with Rust `morton-encoding`.

零堆分配的 CP 空间索引位交错引擎，无分支魔数掩码实现 2D（32×32 → 64-bit）与 3D（21×21×21 → 63-bit）莫顿码编解码及 2D Hilbert 空间填充曲线（`order 1..=16`），适用于四叉树/八叉树、GIS 网格、游戏空间分区与 OLAP 多维聚簇索引（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/morton
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `Point2D` / `Point3D` | `pub struct` | 2D / 3D 无符号整型坐标结构体 |
| `encode2D` / `decode2D` | `(x, y: u32) -> u64` / `(code: u64) -> Point2D` | 2D 32-bit 坐标 ↔ 64-bit 莫顿码无损互转 |
| `encode3D` / `decode3D` | `(x, y, z: u32) -> u64` / `(code: u64) -> Point3D` | 3D 21-bit 坐标 ↔ 63-bit 莫顿码无损互转 |
| `hilbert2DEncode` / `hilbert2DDecode` | `(order: u32, ...)` | 2D Hilbert 曲线编解码（`order` 1..16，相邻点曼哈顿距离恒为 1） |

## License

MIT

# CP 标准库：Geohash 空间网格编码与距离计算手册 (`cap:geohash`)

## 一、 架构总览与设计准则

`cap:geohash` 是 CP 语言官方内置的纯 CP **Geohash 空间网格编码、8 邻域拓扑与 Haversine 球面距离标准库**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**。

既可通过 `import { ... } from "cap:geohash"` 导入，也支持 `import cap from "cap"; cap.geohash.*` 动态懒挂载调用。

### 核心特性与不变量
- **零堆分配栈上编解码**：`encode` 与 `neighbors` 默认返回栈上定长值结构体 `GeohashCode`（12 字节固定缓冲区）与 `Neighbors`（8 个方向 `GeohashCode`），无需手动释放；同时提供 `encodeAlloc(allocator, ...)` 满足需要堆切片的场景。
- **Base32 严格校验**：仅接受标准 Geohash Base32 小写字符集 `"0123456789bcdefghjkmnpqrstuvwxyz"`（排除 `'a'`, `'i'`, `'l'`, `'o'`），空字符串返回 `GeohashError.EmptyGeohash`，非法字符返回 `GeohashError.InvalidGeohashChar`。
- **经度 `±180°` 日界线环绕与极点边界**：`neighbor` / `neighbors` 在跨越 `±180°` 国际日界线时自动环绕至地球另一侧，保持严格拓扑对称性（如 `west(east(h)) == h`）；到达 `±90°` 极点顶行时钳位在当前纬度带，不会从北极跳至南极。
- **64-bit 整型 Geohash（Z-order Morton）**：`encodeInt` / `decodeInt` 支持 `1..63` 位交错编码，数值保持空间 Z 序单调性，适合数据库与 B+ 树范围查询。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/geohash/geohash.cp` | `// Source: cp/lib/geohash/geohash.cp (...)` | `GeohashError`、`LatLon`、`BoundingBox`、`Direction`、`GeohashCode`、`Neighbors` 及全部编解码/邻域/距离 API |
| `lib/geohash/index.cp` | `// Source: cp/lib/geohash/index.cp (...)` | `cap:geohash` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `GeohashError` & 基础类型
- `GeohashError`：`InvalidCoordinate`、`InvalidPrecision`、`InvalidGeohashChar`、`EmptyGeohash`、`OutOfMemory`
- `LatLon { lat: f64, lon: f64 }`：提供 `LatLon.init(lat, lon): LatLon ! GeohashError`
- `BoundingBox { min_lat, max_lat, min_lon, max_lon, center_lat, center_lon, lat_err, lon_err }`：提供 `center(): LatLon`、`contains(lat, lon): bool`
- `Direction`：`North`, `NorthEast`, `East`, `SouthEast`, `South`, `SouthWest`, `West`, `NorthWest`
- `GeohashCode { buf: [12]u8, len: u8 }`：提供 `slice(*_): []const u8`、`copyTo(dst: []u8): []const u8`、`eql(other): bool`、`eqlStr(s): bool`
- `Neighbors { n, ne, e, se, s, sw, w, nw: GeohashCode }`：提供 `all(*_): [8][]const u8`

### 2. 核心函数
- `encode(lat: f64, lon: f64, precision: u8): GeohashCode ! GeohashError`（`lat` ∈ `[-90, 90]`，`lon` ∈ `[-180, 180]`，`precision` ∈ `1..12`）
- `encodeAlloc(allocator: std.mem.Allocator, lat: f64, lon: f64, precision: u8): []u8 ! GeohashError`
- `decode(hash: []const u8): LatLon ! GeohashError`
- `decodeBounds(hash: []const u8): BoundingBox ! GeohashError`
- `neighbor(hash: []const u8, dir: Direction): GeohashCode ! GeohashError`
- `neighbors(hash: []const u8): Neighbors ! GeohashError`
- `encodeInt(lat: f64, lon: f64, bit_depth: u6): u64 ! GeohashError`（`bit_depth` ∈ `1..63`）
- `decodeInt(bits: u64, bit_depth: u6): BoundingBox ! GeohashError`
- `haversineMeters(a: LatLon, b: LatLon): f64`（WGS-84 平均地球半径 `6371000.0` m）
- `precisionForRadiusMeters(radius_m: f64): u8`（返回 `1..12`）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    LatLon,
    Direction,
    encode,
    decode,
    decodeBounds,
    neighbors,
    encodeInt,
    decodeInt,
    haversineMeters,
    precisionForRadiusMeters,
} from "cap:geohash";
import cap from "cap";

pub fn main() {
    let code = encode(37.7749, -122.4194, 5) catch return;
    const s = code.slice(); // "9q8yy"

    const center = decode(s) catch return;
    const box = decodeBounds(s) catch return;
    const inside = box.contains(37.7749, -122.4194); // true

    let nb = neighbors(s) catch return;
    const dirs = nb.all();

    const z_bits = encodeInt(35.6762, 139.6503, 52) catch return;
    const z_box = decodeInt(z_bits, 52) catch return;

    const p1 = LatLon.init(39.9042, 116.4074) catch return;
    const p2 = LatLon.init(31.2304, 121.4737) catch return;
    const dist = haversineMeters(p1, p2);
    const prec = precisionForRadiusMeters(1000.0); // 6

    // 通过 cap.geohash 动态挂载调用
    let dyn = cap.geohash.encode(0.0, 0.0, 5) catch return;
    * = center;
    * = inside;
    * = dirs;
    * = z_box;
    * = dist;
    * = prec;
    * = dyn.slice();
}
```

# CP 标准库：二维码编码与渲染引擎 (`cap:qrcode` / `cap.qrcode`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

## 一、 架构总览

`cap:qrcode` 是 CP 语言官方内置的零外部依赖 ISO/IEC 18004 QR Code Model 2 编码与多格式渲染标准库，支持 Version 1..10 Byte 模式编码、GF(256) Reed-Solomon 纠错、多块交织、BCH 格式/版本信息、8 掩码惩罚评分择优，以及终端 UTF-8 半块、ASCII 与矢量 SVG 渲染（可直接与 `cap:security` 的 TOTP `otpauth://` URI 联动）。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:image"` 与 `import cap from "cap"; cap.qrcode.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/qrcode/ecc.cp` | `// Source: cp/lib/qrcode/ecc.cp (GF(256) Reed-Solomon Error Correction for QR Code)` | GF(256) 算术（本原多项式 `0x11D`）与 `ReedSolomon` 生成多项式及纠错码字计算 |
| `lib/qrcode/encoder.cp` | `// Source: cp/lib/qrcode/encoder.cp (ISO/IEC 18004 QR Code Model 2 Encoder Versions 1..10)` | `QrError`、`QrEccLevel`、`QrMatrix`、`QrCode`（Version 1..10 自动选版、多块交织、8 掩码择优） |
| `lib/qrcode/render.cp` | `// Source: cp/lib/qrcode/render.cp (UTF-8 Half-Block Terminal, ASCII & SVG QR Renderer)` | `QrRenderer`（UTF-8 半块终端渲染、ASCII 网格与水平游程合并 SVG 矢量图） |
| `lib/qrcode/index.cp` | `// Source: cp/lib/qrcode/index.cp (Unified Exports for cap:qrcode)` | `cap:qrcode` 与 `cap.qrcode` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. GF(256) 与 Reed-Solomon 纠错（`ecc.cp`）

- `QR_GF_PRIMITIVE`: `u16 = 0x11D`
- **`ReedSolomon`**（`static struct`）：
  - `gfExp(i: usize): u8`：返回 GF(256) 中 $\alpha^i$
  - `gfLog(a: u8): u8`：返回非零元素的离散对数 $\log_\alpha(a)$
  - `gfMul(a: u8, b: u8): u8`：GF(256) 乘法（模 `0x11D`）
  - `gfPow(base: u8, power: usize): u8`：GF(256) 幂运算
  - `buildGenerator(ecc_len: usize, out_gen: []u8)`：构建 `ecc_len` 次 Reed-Solomon 生成多项式
  - `computeEcc(data: []const u8, ecc_len: usize, out_ecc: []u8)`：计算 `ecc_len` 字节纠错码字并写入 `out_ecc[0..ecc_len]`

### 2. QR Code Model 2 编码器（`encoder.cp`）

- `QR_MIN_VERSION: u8 = 1`，`QR_MAX_VERSION: u8 = 10`
- **`QrError`**：`DataTooLong`、`InvalidVersion`、`OutOfMemory`
- **`QrEccLevel`**：`Low`（~7%）、`Medium`（~15%）、`Quartile`（~25%）、`High`（~30%）
- **`QrMatrix`**：
  - 字段：`version: u8`、`size: usize`（`17 + 4 * version`）、`ecc: QrEccLevel`、`mask: u8`、`modules: []bool`
  - `get(x: usize, y: usize): bool`：查询 `(x, y)` 模块（深色 `true` / 浅色 `false`，越界返回 `false`）
  - `deinit(*_)`：释放 `modules` 堆内存
- **`QrCode`**（`static struct`）：
  - `maxByteCapacity(version: u8, ecc: QrEccLevel): usize`：查询指定版本与纠错级别下的 Byte 模式最大字节容量（无效版本返回 `0`）
  - `encode(alloc, data: []const u8, ecc: QrEccLevel): QrMatrix ! QrError`：自动选择适配 `data.len` 的最小版本（1..10）完成编码
  - `encodeVersion(alloc, data: []const u8, version: u8, ecc: QrEccLevel): QrMatrix ! QrError`：按指定版本编码（越界报 `InvalidVersion`，超容报 `DataTooLong`）

### 3. 多格式渲染器（`render.cp`）

- **`QrRenderer`**（`static struct`）：
  - `toTerminal(alloc, mat: QrMatrix, quiet_zone: usize): ([]u8) ! QrError`：使用 UTF-8 半块字符（`▀` / `▄` / `█` / ` `）将两行模块合并为一行紧凑终端输出
  - `toAscii(alloc, mat: QrMatrix, quiet_zone: usize): ([]u8) ! QrError`：输出等宽方正 ASCII 网格（`##` 与 `  `）
  - `toSvg(alloc, mat: QrMatrix, cell_px: usize, quiet_zone: usize): ([]u8) ! QrError`：输出规范 `<svg xmlns="http://www.w3.org/2000/svg" viewBox="...">` 矢量图（水平连续深色模块自动合并为单条 `<path>` 线段）

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { QrCode, QrEccLevel, QrRenderer } from "cap:image";
import { Totp, TotpConfig } from "cap:security";

pub fn demoOtpQr(): !void {
    const alloc = std.heap.page_allocator;
    const uri = Totp.buildOtpAuthUri(
        alloc,
        "CPAuth",
        "alice@cp-lang.org",
        "JBSWY3DPEHPK3PXP",
        TotpConfig.initDefault(),
    )?;
    defer alloc.free(uri);

    let mat = QrCode.encode(alloc, uri, .Medium)?;
    defer mat.deinit();

    const term = QrRenderer.toTerminal(alloc, mat, 2)?;
    defer alloc.free(term);

    const svg = cap.qrcode.QrRenderer.toSvg(alloc, mat, 4, 2)?;
    defer alloc.free(svg);
}
```

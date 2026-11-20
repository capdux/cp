# CP 标准库：一维条码编码与矢量/终端渲染引擎 (`cap:barcode` / `cap.barcode`)

## 一、 架构总览

`cap:barcode` 是 CP 语言官方内置的工业级、纯 CP、零 C 依赖一维条码编码与渲染标准库，适用于物流仓储、零售 POS 与票据生成（可与 `cap:xml`、`cap:svg`、`cap:qrcode`、`cap:pdf` 协同）。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:barcode"` 与 `import cap from "cap"; cap.barcode.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/barcode/barcode.cp` | `// Source: cp/lib/barcode/barcode.cp (...)` | `BarcodeError`、`BarcodeFormat`、`BarcodeBitPattern`、`BarcodeEncoder`（Code 128 / EAN-13 / UPC-A）与 `BarcodeRenderer`（SVG / ASCII） |
| `lib/barcode/index.cp` | `// Source: cp/lib/barcode/index.cp (...)` | `cap:barcode` 与 `cap.barcode` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 错误与基础类型

- **`BarcodeError`**：`EmptyInput`、`InvalidCharacter`、`InvalidLength`、`InvalidChecksum`、`OutOfMemory`
- **`BarcodeFormat`**：`Code128`、`Ean13`、`UpcA`
- **`BarcodeBitPattern`**：
  - 字段：`format: BarcodeFormat`、`text: []const u8`、`modules: []bool`（`true` = 黑条，`false` = 白空）
  - `deinit(*_)`：释放 `modules` 与 `text` 堆内存

### 2. 条码编码器 (`BarcodeEncoder`)

- `ean13CheckDigit(digits12: []const u8): u8 ! BarcodeError`：计算 12 位 EAN-13 校验位（返回 `0..9`）
- `verifyEan13(digits13: []const u8): bool`：校验 13 位 EAN-13 条码校验位
- `upcaCheckDigit(digits11: []const u8): u8 ! BarcodeError`：计算 11 位 UPC-A 校验位（返回 `0..9`）
- `verifyUpca(digits12: []const u8): bool`：校验 12 位 UPC-A 条码校验位
- `encodeEan13(alloc: std.mem.Allocator, digits: []const u8): BarcodeBitPattern ! BarcodeError`：
  - 接收 12 位（自动补第 13 位校验位）或 13 位（严格校验，不符返回 `InvalidChecksum`）；
  - 输出标准 **95 模块**序列：`101`（左护栏）+ `6*7`（L/G 奇偶模式）+ `01010`（中护栏）+ `6*7`（R 模式）+ `101`（右护栏）。
- `encodeUpcA(alloc: std.mem.Allocator, digits: []const u8): BarcodeBitPattern ! BarcodeError`：
  - 接收 11 位或 12 位数字，校验后以首位 `'0'` 前缀兼容 EAN-13 标准 **95 模块**输出。
- `encodeCode128(alloc: std.mem.Allocator, text: []const u8): BarcodeBitPattern ! BarcodeError`：
  - 支持完整 ASCII `0..127`（非 ASCII 返回 `InvalidCharacter`）；
  - 自动最优 A/B/C 子集切换（连续 `>= 4` 位数字自动切入 Code C 双数字压缩模式）；
  - 附带 Mod 103 校验符号与 13 模块 Stop 符号 `1100011101011`（总模块数 = `11 * symbols + 2`）。

### 3. 矢量与终端渲染器 (`BarcodeRenderer`)

- `toSvg(alloc, pattern: BarcodeBitPattern, bar_width_px: usize, height_px: usize, show_text: bool): ([]u8) ! BarcodeError`：
  - 输出规范 `<svg xmlns="http://www.w3.org/2000/svg" viewBox="...">` 矢量图（可被 `cap:xml` 直接解析），自动合并连续黑条为单个 `<rect>` 并对文本执行 XML 转义。
- `toAscii(alloc, pattern: BarcodeBitPattern): ([]u8) ! BarcodeError`：
  - 输出多行 UTF-8 方块条形图（`█` 与 ` `）。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { BarcodeEncoder, BarcodeRenderer } from "cap:barcode";

pub fn demoBarcode(): !void {
    const alloc = std.heap.page_allocator;

    let pat = BarcodeEncoder.encodeCode128(alloc, "PKG-20261120")?;
    defer pat.deinit();

    const svg = BarcodeRenderer.toSvg(alloc, pat, 2, 60, true)?;
    defer alloc.free(svg);

    let ean = cap.barcode.BarcodeEncoder.encodeEan13(alloc, "4006381333931")?;
    defer ean.deinit();
    const ascii = cap.barcode.BarcodeRenderer.toAscii(alloc, ean)?;
    defer alloc.free(ascii);
}
```

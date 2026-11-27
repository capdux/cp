[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jsbarcode

Zero-allocation 1D barcode encoder & SVG renderer (JsBarcode / barcoders parity: EAN-13, UPC-A, Code 128B, Code 39 with checksum verification and crisp SVG output) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:jsbarcode` ([https://github.com/lindell/JsBarcode](https://github.com/lindell/JsBarcode))（原库名 `jsbarcode` → CP 包名 `jsbarcode`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `jsbarcode` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add jsbarcode
```

```cp
import {
    Barcode,
    ean13Checksum,
    isValidEan13,
    upcAChecksum,
    isValidUpcA,
    eanLCode,
    eanGCode,
    eanRCode,
} from "jsbarcode";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Barcode` | `class` | 字段: `modules: [MAX_MODULES]u8, len: usize, ok: bool` · 方法: `empty, pushBit, pushBits, pushWidths, bitAt, renderAscii, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ean13Checksum` | `(digits12: string): i8` | 调用 `ean13Checksum` 并返回 `i8`。 |
| `isValidEan13` | `(code: string): bool` | 执行 `isValidEan13` 条件判定，返回 `bool`。 |
| `upcAChecksum` | `(digits11: string): i8` | 调用 `upcAChecksum` 并返回 `i8`。 |
| `isValidUpcA` | `(code: string): bool` | 执行 `isValidUpcA` 条件判定，返回 `bool`。 |
| `eanLCode` | `(d: u8): string` | 调用 `eanLCode` 并返回 `string`。 |
| `eanGCode` | `(d: u8): string` | 调用 `eanGCode` 并返回 `string`。 |
| `eanRCode` | `(d: u8): string` | 调用 `eanRCode` 并返回 `string`。 |
| `eanParityPattern` | `(first_digit: u8): string` | 调用 `eanParityPattern` 并返回 `string`。 |
| `encodeEan13` | `(code: string): Barcode` | 通过 `encodeEan13` 编码或格式化输出，返回 `Barcode`。 |
| `encodeUpcA` | `(code: string): Barcode` | 通过 `encodeUpcA` 编码或格式化输出，返回 `Barcode`。 |
| `code128Pattern` | `(val: usize): string` | 调用 `code128Pattern` 并返回 `string`。 |
| `encodeCode128B` | `(text: string): Barcode` | 通过 `encodeCode128B` 编码或格式化输出，返回 `Barcode`。 |
| `code39Pattern` | `(c: u8): string` | 调用 `code39Pattern` 并返回 `string`。 |
| `encodeCode39` | `(text: string): Barcode` | 通过 `encodeCode39` 编码或格式化输出，返回 `Barcode`。 |

## 最小调用示例

```cp
import { isValidEan13 } from "jsbarcode";

pub fn main() {
    let result = isValidEan13("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

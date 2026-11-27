[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hexdump-nodejs

Zero-allocation canonical hexdump -C 16-byte-per-line binary debugger, spaced hex encoder, and first-diff locator in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hexdump-nodejs` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:pretty-hex` ([https://github.com/wolandr/pretty-hex](https://github.com/wolandr/pretty-hex))（原库名 `hexdump-nodejs` → CP 包名 `hexdump-nodejs`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hexdump-nodejs` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hexdump-nodejs
```

```cp
import {
    buildPrintableLut,
    formatLine,
    formatDump,
    encodeHexSpaced,
    firstDiffOffset,
} from "hexdump-nodejs";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `buildPrintableLut` | `(): [256]u8` | 调用 `buildPrintableLut` 并返回 `[256]u8`。 |
| `formatLine` | `(data: &[]u8, base_offset: usize, out: []u8): usize` | 通过 `formatLine` 编码或格式化输出，返回 `usize`。 |
| `formatDump` | `(data: &[]u8, out: []u8): usize` | 通过 `formatDump` 编码或格式化输出，返回 `usize`。 |
| `encodeHexSpaced` | `(data: &[]u8, uppercase: bool, out: []u8): usize` | 通过 `encodeHexSpaced` 编码或格式化输出，返回 `usize`。 |
| `firstDiffOffset` | `(a: &[]u8, b: &[]u8): i64` | 调用 `firstDiffOffset` 并返回 `i64`。 |

## 最小调用示例

```cp
import { formatLine } from "hexdump-nodejs";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let mut buf_1: [256]u8 = undefined;
    let result = formatLine(out_buf[0..256], 0, buf_1[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

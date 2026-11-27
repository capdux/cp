[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# pretty-bytes

Pure CP zero-allocation human-readable byte and bit size formatter supporting SI/IEC, bits, signed, and compact modes (npm pretty-bytes parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-bytes` ([https://github.com/sindresorhus/pretty-bytes](https://github.com/sindresorhus/pretty-bytes))

| 元信息 | 值 |
| --- | --- |
| 包名 | `pretty-bytes` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add pretty-bytes
```

```cp
import {
    PrettyBytesOpts,
    pbUnitStr,
    pbAppendStr,
    pbAppendByte,
    pbAppendU64,
    prettyBytesWith,
    prettyBytes,
} from "pretty-bytes";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PrettyBytesOpts` | `class` | 字段: `binary: bool, bits: bool, signed_val: bool, space: bool, precision: u8` · 方法: `defaults` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pbUnitStr` | `(idx: usize, binary: bool, bits: bool): string` | 调用 `pbUnitStr` 并返回 `string`。 |
| `pbAppendStr` | `(out: []u8, pos: &mut usize, s: string): bool` | 调用 `pbAppendStr` 并返回 `bool`。 |
| `pbAppendByte` | `(out: []u8, pos: &mut usize, ch: u8): bool` | 调用 `pbAppendByte` 并返回 `bool`。 |
| `pbAppendU64` | `(out: []u8, pos: &mut usize, v: u64): bool` | 调用 `pbAppendU64` 并返回 `bool`。 |
| `prettyBytesWith` | `(num: f64, opts: PrettyBytesOpts, out: []u8): usize` | 调用 `prettyBytesWith` 并返回 `usize`。 |
| `prettyBytes` | `(num: f64, out: []u8): usize` | 调用 `prettyBytes` 并返回 `usize`。 |

## 最小调用示例

```cp
import { pbAppendStr } from "pretty-bytes";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = pbAppendStr(out_buf[0..256], /* pos: &mut usize */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

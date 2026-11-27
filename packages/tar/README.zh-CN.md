[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tar

Zero-heap POSIX USTAR 512-byte tar archive header codec and streaming cursor (crates.io tar equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tar` ([https://github.com/alexcrichton/tar-rs](https://github.com/alexcrichton/tar-rs))（原库名 `tar` → CP 包名 `tar`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `tar` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tar
```

```cp
import {
    TarHeader,
    TarEntrySpan,
    TarArchiveCursor,
    tarEmptyHeader,
    tarWriteOctal,
    tarParseOctal,
    tarComputeChecksum,
    tarIsUstarMagic,
} from "tar";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TarHeader` | `class` | 字段: `name: string, mode: u32, uid: u32, gid: u32, size: u64, mtime: u64, +4 项` |
| `TarEntrySpan` | `class` | 字段: `header: TarHeader, data_offset: usize, data_len: usize` |
| `TarArchiveCursor` | `class` | 字段: `offset: usize, finished: bool` · 方法: `init, reset, nextEntry` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `tarEmptyHeader` | `(): TarHeader` | 调用 `tarEmptyHeader` 并返回 `TarHeader`。 |
| `tarWriteOctal` | `(out: []u8, offset: usize, digits: usize, val: u64): bool` | 调用 `tarWriteOctal` 并返回 `bool`。 |
| `tarParseOctal` | `(buf: []u8, offset: usize, width: usize, require_digit: bool): u64?` | 调用 `tarParseOctal` 并返回 `u64?`。 |
| `tarComputeChecksum` | `(block512: []u8): u32` | 调用 `tarComputeChecksum` 并返回 `u32`。 |
| `tarIsUstarMagic` | `(block512: []u8): bool` | 调用 `tarIsUstarMagic` 并返回 `bool`。 |
| `isZeroBlock` | `(block512: []u8): bool` | 执行 `isZeroBlock` 条件判定，返回 `bool`。 |
| `tarPaddedSize` | `(raw_size: u64): u64` | 调用 `tarPaddedSize` 并返回 `u64`。 |
| `encodeTarHeader` | `(name: string, size: u64, mode: u32, mtime: u64, typeflag: u8, out512: []u8): bool` | 通过 `encodeTarHeader` 编码或格式化输出，返回 `bool`。 |
| `parseTarHeader` | `(block512: []u8): TarHeader` | 通过 `parseTarHeader` 解析输入数据，返回 `TarHeader`。 |

## 最小调用示例

```cp
import { encodeTarHeader } from "tar";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = encodeTarHeader("example", 0, 0, 0, 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

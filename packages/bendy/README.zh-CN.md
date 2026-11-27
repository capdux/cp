[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# bendy

BitTorrent BEP-3 deterministic binary Bencode streaming encoder, zero-copy decoder, validator, and dict lookup.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bendy` ([https://github.com/P3KI/bendy](https://github.com/P3KI/bendy))（原库名 `bendy` → CP 包名 `bendy`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `bendy` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add bendy
```

```cp
import {
    BencodeIntResult,
    BencodeBytesResult,
    BencodeWriter,
    peekKind,
    decodeInt,
    decodeBytes,
    compareBytes,
    parseValueEnd,
} from "bendy";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `BencodeIntResult` | `class` | 字段: `value: i64, next_pos: usize, ok: bool` |
| `BencodeBytesResult` | `class` | 字段: `start: usize, len: usize, next_pos: usize, ok: bool` |
| `BencodeWriter` | `class` | 字段: `buf: []u8, pos: usize, overflow: bool` · 方法: `init, writeByte, writeSlice, writeInt, writeBytes, beginList, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `peekKind` | `(src: &[]u8, pos: usize): u8` | 调用 `peekKind` 并返回 `u8`。 |
| `decodeInt` | `(src: &[]u8, pos: usize): BencodeIntResult` | 通过 `decodeInt` 解析输入数据，返回 `BencodeIntResult`。 |
| `decodeBytes` | `(src: &[]u8, pos: usize): BencodeBytesResult` | 通过 `decodeBytes` 解析输入数据，返回 `BencodeBytesResult`。 |
| `compareBytes` | `(a: &[]u8, b: &[]u8): i32` | 调用 `compareBytes` 并返回 `i32`。 |
| `parseValueEnd` | `(src: &[]u8, pos: usize, depth: usize): i64` | 通过 `parseValueEnd` 解析输入数据，返回 `i64`。 |
| `validate` | `(src: &[]u8): bool` | 校验输入合法性，返回 `bool`。 |
| `dictLookup` | `(src: &[]u8, key: &[]u8): BencodeBytesResult` | 调用 `dictLookup` 并返回 `BencodeBytesResult`。 |

## 最小调用示例

```cp
import { decodeInt } from "bendy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = decodeInt(out_buf[0..256], 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

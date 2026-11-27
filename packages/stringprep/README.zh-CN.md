[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# stringprep

Zero-heap RFC 3454 / RFC 4013 SASLprep & Nameprep string preparation for SCRAM-SHA-256, PostgreSQL, and LDAP in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:stringprep` ([https://github.com/sfackler/rust-stringprep](https://github.com/sfackler/rust-stringprep))

| 元信息 | 值 |
| --- | --- |
| 包名 | `stringprep` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add stringprep
```

```cp
import {
    PrepResult,
    DecCp,
    decodeUtf8,
    encodeUtf8,
    isMappedToNothing,
    isNonAsciiSpace,
    isSaslProhibited,
    saslprep,
} from "stringprep";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PrepResult` | `class` | 字段: `len: usize, ok: bool` |
| `DecCp` | `class` | 字段: `cp: u32, width: usize, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `decodeUtf8` | `(s: string, i: usize): DecCp` | 通过 `decodeUtf8` 解析输入数据，返回 `DecCp`。 |
| `encodeUtf8` | `(cp: u32, out: []u8, pos: usize): i64` | 通过 `encodeUtf8` 编码或格式化输出，返回 `i64`。 |
| `isMappedToNothing` | `(cp: u32): bool` | 执行 `isMappedToNothing` 条件判定，返回 `bool`。 |
| `isNonAsciiSpace` | `(cp: u32): bool` | 执行 `isNonAsciiSpace` 条件判定，返回 `bool`。 |
| `isSaslProhibited` | `(cp: u32): bool` | 执行 `isSaslProhibited` 条件判定，返回 `bool`。 |
| `saslprep` | `(input: string, out: []u8): PrepResult` | 调用 `saslprep` 并返回 `PrepResult`。 |
| `nameprepAscii` | `(input: string, out: []u8): PrepResult` | 调用 `nameprepAscii` 并返回 `PrepResult`。 |

## 最小调用示例

```cp
import { decodeUtf8 } from "stringprep";

pub fn main() {
    let result = decodeUtf8("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

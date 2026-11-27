[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ksuid

Zero-heap Segment KSUID (20-byte K-Sortable Unique IDentifier) 27-character Base62 encoder, decoder, validator, and comparator.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ksuid` ([https://github.com/segmentio/ksuid](https://github.com/segmentio/ksuid)) / `crates.io:svix-ksuid` ([https://github.com/svix/rust-ksuid](https://github.com/svix/rust-ksuid))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ksuid` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ksuid
```

```cp
import {
    Ksuid,
    b62Char,
    b62Val,
    fromParts,
    timestampSecs,
    encode,
    decode,
    isValidKsuid,
} from "ksuid";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ksuid` | `class` | 字段: `bytes: [20]u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `b62Char` | `(v: u8): u8` | 调用 `b62Char` 并返回 `u8`。 |
| `b62Val` | `(c: u8): i32` | 调用 `b62Val` 并返回 `i32`。 |
| `fromParts` | `(unix_secs: u64, payload: [16]u8): Ksuid` | 调用 `fromParts` 并返回 `Ksuid`。 |
| `timestampSecs` | `(k: Ksuid): u64` | 调用 `timestampSecs` 并返回 `u64`。 |
| `encode` | `(k: Ksuid, out: []u8): usize` | 通过 `encode` 编码或格式化输出，返回 `usize`。 |
| `decode` | `(s: string, out: &mut Ksuid): bool` | 通过 `decode` 解析输入数据，返回 `bool`。 |
| `isValidKsuid` | `(s: string): bool` | 执行 `isValidKsuid` 条件判定，返回 `bool`。 |
| `compare` | `(a: Ksuid, b: Ksuid): i32` | 调用 `compare` 并返回 `i32`。 |

## 最小调用示例

```cp
import { decode } from "ksuid";

pub fn main() {
    let result = decode("example", /* out: &mut Ksuid */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

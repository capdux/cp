[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# siphasher

Zero-allocation SipHash-2-4 and SipHash-1-3 keyed streaming PRF/hashers (HashDoS-resistant) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`siphasher`](https://github.com/jedisct1/rust-siphash)（`MIT OR Apache-2.0`）（原库名 `siphasher`，CP 包名 `siphasher`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `siphasher` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add siphasher
```

```cp
import {
    SipState,
    SipHasher24,
    SipHasher13,
    addWrap64,
    rotl64,
    sipRound,
    loadLe64,
    loadLe64Str,
} from "siphasher";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SipState` | `class` | 字段: `v0: u64, v1: u64, v2: u64, v3: u64` |
| `SipHasher24` | `class` | 字段: `v0: u64, v1: u64, v2: u64, v3: u64, buf: [8]u8, buf_len: usize, +1 项` · 方法: `init, compress, write, finish` |
| `SipHasher13` | `class` | 字段: `v0: u64, v1: u64, v2: u64, v3: u64, buf: [8]u8, buf_len: usize, +1 项` · 方法: `init, compress, write, finish` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `addWrap64` | `(a: u64, b: u64): u64` | 写入或更新状态数据。 |
| `rotl64` | `(x: u64, r: u64): u64` | 调用 `rotl64` 并返回 `u64`。 |
| `sipRound` | `(s: SipState): SipState` | 调用 `sipRound` 并返回 `SipState`。 |
| `loadLe64` | `(buf: []u8, offset: usize): u64` | 调用 `loadLe64` 并返回 `u64`。 |
| `loadLe64Str` | `(s: string, offset: usize): u64` | 调用 `loadLe64Str` 并返回 `u64`。 |
| `siphash24` | `(k0: u64, k1: u64, data: string): u64` | 调用 `siphash24` 并返回 `u64`。 |
| `siphash13` | `(k0: u64, k1: u64, data: string): u64` | 调用 `siphash13` 并返回 `u64`。 |

## 最小调用示例

```cp
import { loadLe64Str } from "siphasher";

pub fn main() {
    let result = loadLe64Str("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

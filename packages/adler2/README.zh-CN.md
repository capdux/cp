[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# adler2

面向 CP 语言的零堆分配 RFC 1950 Adler-32 校验和引擎，支持流式更新、O(1) 滚动窗口更新（rsync 风格）与分块校验和合并。

对齐 Rust crates.io 源包 [`adler2`](https://github.com/oyvindln/adler2)（MIT）。

## 安装与导入

```bash
cap add adler2
```

```cp
import {
    ADLER32_MOD,
    ADLER32_NMAX,
    Adler32,
    adler32,
    adler32Slice,
    adler32Str,
    combineAdler32,
    adler32Combine,
} from "adler2";
```

## 核心 API

### 常量

| 符号 | 类型 | 说明 |
| --- | --- | --- |
| `ADLER32_MOD` | `u32 = 65521` | 小于 65536 的最大素数（RFC 1950 模数）。 |
| `ADLER32_NMAX` | `usize = 5552` | 延迟取模批处理单轮最大字节数（保证 32 位不溢出）。 |

### `Adler32` 流式校验类

| 成员 | 签名 | 说明 |
| --- | --- | --- |
| 字段 | `s1: u32, s2: u32` | Adler-32 内部累加器（初始 `s1 = 1, s2 = 0`）。 |
| `Adler32.init` / `new` | `(): Adler32` | 创建初始值为 `1` 的 Adler-32 状态机。 |
| `Adler32.fromValue` / `fromChecksum` | `(value: u32): Adler32` | 从已有 32 位 Adler-32 校验和恢复状态。 |
| `update` / `writeSlice` | `(*self, data: []const u8)` | 输入只读字节切片更新校验和。 |
| `writeStr` | `(*self, s: string)` | 输入字符串更新校验和。 |
| `updateByte` / `writeByte` | `(*self, b: u8)` | 输入单字节更新校验和。 |
| `roll` | `(*self, window_len: usize, out_byte: u8, in_byte: u8)` | O(1) 滚动窗口更新：移出 `out_byte`、移入 `in_byte`（窗口长度 `window_len`）。 |
| `finalize` / `checksum` | `(): u32` | 返回 `(s2 << 16) | s1`，不修改内部状态。 |
| `reset` | `(*self)` | 重置状态为 `(s1 = 1, s2 = 0)`。 |

### 一次性与合并函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `adler32` / `adler32Slice` | `(data: []const u8): u32` | 一次性计算字节切片的 Adler-32 校验和。 |
| `adler32Str` | `(s: string): u32` | 一次性计算字符串的 Adler-32 校验和。 |
| `combineAdler32` / `adler32Combine` | `(adler1: u32, adler2: u32, len2: u64): u32` | 根据第二段字节长度 `len2` 合并两个连续数据块的 Adler-32 校验和。 |

## 使用示例

### 1. 一次性与流式计算

```cp
import { print } from "cap:io";
import { Adler32, adler32Str } from "adler2";

pub fn main() {
    let direct = adler32Str("Wikipedia");
    let h = Adler32.new();
    h.writeStr("Wiki");
    h.writeStr("pedia");
    print("direct={}, stream={}\n", direct, h.checksum()); // 均为 0x11e60398
}
```

### 2. 滚动窗口校验和

```cp
import { print } from "cap:io";
import { Adler32 } from "adler2";

pub fn main() {
    let h = Adler32.new();
    h.writeStr("abcd");
    h.roll(4, 97, 101); // 窗口从 "abcd" 滑动至 "bcde"
    print("rolled={}\n", h.checksum());
}
```

### 3. 并行分块校验和合并

```cp
import { print } from "cap:io";
import { adler32Str, combineAdler32 } from "adler2";

pub fn main() {
    let c1 = adler32Str("hello ");
    let c2 = adler32Str("world");
    let combined = combineAdler32(c1, c2, 5);
    print("combined={}, full={}\n", combined, adler32Str("hello world"));
}
```

## 与源包差异与零拷贝安全亮点

- **极简零堆状态**：`Adler32` 仅占 8 字节栈空间，配合 `ADLER32_NMAX = 5552` 延迟取模优化实现高吞吐。
- **内置滚动哈希与合并**：在对齐 Rust `adler2` 流式接口的基础上，补充 O(1) `roll` 滚动窗口与 `combineAdler32` 合并能力。
- **无副作用读取**：`checksum()` / `finalize()` 为只读方法，支持在流式处理中多次读取中间校验值。

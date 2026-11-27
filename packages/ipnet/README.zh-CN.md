[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ipnet

Zero-heap pure-CP IPv4/CIDR subnet calculation engine (parse, format, masks, containment, supernet, host count — crates.io ipnet equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ipnet` ([https://github.com/krisprice/ipnet](https://github.com/krisprice/ipnet))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ipnet` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ipnet
```

```cp
import {
    Ipv4Net,
    ipnParseIpv4Range,
    ipnParsePrefixRange,
    ipnWriteU8Dec,
    ipnNetmask,
    parseIpv4,
    formatIpv4,
} from "ipnet";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Ipv4Net` | `class` | 字段: `addr: u32, prefix_len: u8` · 方法: `new, parse, netmask, hostmask, network, broadcast, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ipnParseIpv4Range` | `(s: string, start: usize, end: usize): u32?` | 调用 `ipnParseIpv4Range` 并返回 `u32?`。 |
| `ipnParsePrefixRange` | `(s: string, start: usize, end: usize): u8?` | 调用 `ipnParsePrefixRange` 并返回 `u8?`。 |
| `ipnWriteU8Dec` | `(v: u8, out: []u8, pos: usize): usize` | 调用 `ipnWriteU8Dec` 并返回 `usize`。 |
| `ipnNetmask` | `(prefix_len: u8): u32` | 调用 `ipnNetmask` 并返回 `u32`。 |
| `parseIpv4` | `(s: string): u32?` | 通过 `parseIpv4` 解析输入数据，返回 `u32?`。 |
| `formatIpv4` | `(ip: u32, out: []u8): usize` | 通过 `formatIpv4` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parseIpv4 } from "ipnet";

pub fn main() {
    let result = parseIpv4("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

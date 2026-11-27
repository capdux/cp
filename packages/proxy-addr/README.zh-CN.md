[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# proxy-addr

Pure CP zero-allocation reverse-proxy client IP resolver and trusted proxy chain validator (npm proxy-addr parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-addr` ([https://github.com/jshttp/proxy-addr](https://github.com/jshttp/proxy-addr))

| 元信息 | 值 |
| --- | --- |
| 包名 | `proxy-addr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add proxy-addr
```

```cp
import {
    IpSpan,
    PaIpv4,
    PaIpv6Info,
    paTrimBounds,
    paStripV4Mapped,
    paParseIpv4,
    paParseIpv6Info,
    isLoopback,
} from "proxy-addr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IpSpan` | `class` | 字段: `start: usize, len: usize` |
| `PaIpv4` | `class` | 字段: `ok: bool, o0: u8, o1: u8, o2: u8, o3: u8` |
| `PaIpv6Info` | `class` | 字段: `ok: bool, is_loopback: bool, first_word: u16` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `paTrimBounds` | `(s: string): IpSpan` | 调用 `paTrimBounds` 并返回 `IpSpan`。 |
| `paStripV4Mapped` | `(s: string): string` | 调用 `paStripV4Mapped` 并返回 `string`。 |
| `paParseIpv4` | `(s: string): PaIpv4` | 调用 `paParseIpv4` 并返回 `PaIpv4`。 |
| `paParseIpv6Info` | `(s: string): PaIpv6Info` | 调用 `paParseIpv6Info` 并返回 `PaIpv6Info`。 |
| `isLoopback` | `(ip: string): bool` | 执行 `isLoopback` 条件判定，返回 `bool`。 |
| `isLinkLocal` | `(ip: string): bool` | 执行 `isLinkLocal` 条件判定，返回 `bool`。 |
| `isUniqueLocal` | `(ip: string): bool` | 执行 `isUniqueLocal` 条件判定，返回 `bool`。 |
| `isTrustedBuiltin` | `(ip: string, preset: u8): bool` | 执行 `isTrustedBuiltin` 条件判定，返回 `bool`。 |
| `parseXffAddresses` | `(remote_addr: string, xff_header: string, out: []IpSpan): usize` | 通过 `parseXffAddresses` 解析输入数据，返回 `usize`。 |
| `paCopyToOut` | `(src: string, out: []u8): usize` | 调用 `paCopyToOut` 并返回 `usize`。 |
| `resolveClientIp` | `(remote_addr: string, xff_header: string, trust_preset: u8, max_hops: usize, out: []u8): usize` | 调用 `resolveClientIp` 并返回 `usize`。 |

## 最小调用示例

```cp
import { isLoopback } from "proxy-addr";

pub fn main() {
    let result = isLoopback("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

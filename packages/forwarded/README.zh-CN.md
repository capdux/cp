[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# forwarded

Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:forwarded-header-value` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `forwarded-header-value` → CP 包名 `forwarded`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `forwarded` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add forwarded
```

```cp
import {
    ForwardedHop,
    ForwardedChain,
    unquote,
    stripAddr,
    parseForwardedElement,
    parseForwarded,
    parseXForwardedFor,
    extractClientIp,
} from "forwarded";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ForwardedHop` | `class` | 字段: `for_addr: string, by_addr: string, host: string, proto: string` · 方法: `empty` |
| `ForwardedChain` | `class` | 字段: `hops: [MAX_HOPS]ForwardedHop, count: usize` · 方法: `empty, hopAt, clientIp, clientProto, clientHost, ipAtTrustDepth` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `unquote` | `(raw: string): string` | 调用 `unquote` 并返回 `string`。 |
| `stripAddr` | `(raw: string): string` | 调用 `stripAddr` 并返回 `string`。 |
| `parseForwardedElement` | `(elem: string): ForwardedHop` | 通过 `parseForwardedElement` 解析输入数据，返回 `ForwardedHop`。 |
| `parseForwarded` | `(header: string): ForwardedChain` | 通过 `parseForwarded` 解析输入数据，返回 `ForwardedChain`。 |
| `parseXForwardedFor` | `(xff: string, remote_addr: string): ForwardedChain` | 通过 `parseXForwardedFor` 解析输入数据，返回 `ForwardedChain`。 |
| `extractClientIp` | `(xff: string, remote_addr: string, trusted_hops: usize): string` | 调用 `extractClientIp` 并返回 `string`。 |
| `isPrivateIpv4` | `(s: string): bool` | 执行 `isPrivateIpv4` 条件判定，返回 `bool`。 |
| `isPrivateIp` | `(ip: string): bool` | 执行 `isPrivateIp` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { parseForwardedElement } from "forwarded";

pub fn main() {
    let result = parseForwardedElement("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

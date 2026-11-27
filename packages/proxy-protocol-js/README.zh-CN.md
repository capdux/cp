[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# proxy-protocol-js

Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary) parser and encoder for L4 client IP preservation.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-protocol-js` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp)) / `crates.io:ppp` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `proxy-protocol-js` → CP 包名 `proxy-protocol-js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `proxy-protocol-js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add proxy-protocol-js
```

```cp
import {
    ProxyV1Header,
    ProxyV2Header,
    isV1Prefix,
    isV2Signature,
    parsePortStrict,
    parseV1,
    appendSlice,
    appendU16,
} from "proxy-protocol-js";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ProxyV1Header` | `class` | 字段: `proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, consumed: usize` |
| `ProxyV2Header` | `class` | 字段: `is_proxy: bool, family: u8, transport: u8, payload_len: u16, src_ipv4: [4]u8, dst_ipv4: [4]u8, +3 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isV1Prefix` | `(data: &[]u8): bool` | 执行 `isV1Prefix` 条件判定，返回 `bool`。 |
| `isV2Signature` | `(data: &[]u8): bool` | 执行 `isV2Signature` 条件判定，返回 `bool`。 |
| `parsePortStrict` | `(s: string): i32` | 通过 `parsePortStrict` 解析输入数据，返回 `i32`。 |
| `parseV1` | `(data: string, out: &mut ProxyV1Header): i32` | 通过 `parseV1` 解析输入数据，返回 `i32`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `appendU16` | `(out: []u8, pos: usize, v: u16): i64` | 调用 `appendU16` 并返回 `i64`。 |
| `encodeV1` | `(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize` | 通过 `encodeV1` 编码或格式化输出，返回 `usize`。 |
| `parseV2` | `(data: &[]u8, out: &mut ProxyV2Header): i32` | 通过 `parseV2` 解析输入数据，返回 `i32`。 |
| `encodeV2Ipv4` | `(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize` | 通过 `encodeV2Ipv4` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { parsePortStrict } from "proxy-protocol-js";

pub fn main() {
    let result = parsePortStrict("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

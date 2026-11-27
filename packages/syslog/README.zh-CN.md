[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# syslog

Zero-heap RFC 5424 Syslog message parser and formatter with PRI facility/severity codec, structured data skipping, and NILVALUE handling.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:syslog` ([https://github.com/Geal/rust-syslog](https://github.com/Geal/rust-syslog))

| 元信息 | 值 |
| --- | --- |
| 包名 | `syslog` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add syslog
```

```cp
import {
    SyslogMessage,
    TokenSpan,
    encodePri,
    decodeFacility,
    decodeSeverity,
    readSpToken,
    parseRfc5424,
    appendSlice,
} from "syslog";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SyslogMessage` | `class` | 字段: `facility: u8, severity: u8, version: u8, ts_start: usize, ts_len: usize, host_start: usize, +10 项` |
| `TokenSpan` | `class` | 字段: `start: usize, len: usize, next_pos: usize, ok: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `encodePri` | `(facility: u8, severity: u8): u8` | 通过 `encodePri` 编码或格式化输出，返回 `u8`。 |
| `decodeFacility` | `(pri: u8): u8` | 通过 `decodeFacility` 解析输入数据，返回 `u8`。 |
| `decodeSeverity` | `(pri: u8): u8` | 通过 `decodeSeverity` 解析输入数据，返回 `u8`。 |
| `readSpToken` | `(raw: &[]u8, pos: usize): TokenSpan` | 调用 `readSpToken` 并返回 `TokenSpan`。 |
| `parseRfc5424` | `(raw: &[]u8): SyslogMessage` | 通过 `parseRfc5424` 解析输入数据，返回 `SyslogMessage`。 |
| `appendSlice` | `(dst: []u8, pos: usize, s: &[]u8): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `appendFieldOrNil` | `(dst: []u8, pos: usize, field: &[]u8): i64` | 调用 `appendFieldOrNil` 并返回 `i64`。 |
| `appendU8Dec` | `(dst: []u8, pos: usize, val: u8): i64` | 调用 `appendU8Dec` 并返回 `i64`。 |
| `formatRfc5424` | `(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64` | 通过 `formatRfc5424` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { encodePri } from "syslog";

pub fn main() {
    let result = encodePri(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

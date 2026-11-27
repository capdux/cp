[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# retry-after

Zero-heap RFC 9110 HTTP Retry-After header parser and formatter (delay-seconds & HTTP-date) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:retry-after` ([https://github.com/jwilm/retry-after](https://github.com/jwilm/retry-after))

| 元信息 | 值 |
| --- | --- |
| 包名 | `retry-after` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add retry-after
```

```cp
import {
    RetryAfter,
    parseUint,
    parseMonth,
    isLeapYear,
    daysInMonth,
    daysFromCivil,
    parseHttpDate,
    parseRetryAfter,
} from "retry-after";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `RetryAfter` | `class` | 字段: `kind: u8, delay_seconds: u64, epoch_seconds: i64, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `parseUint` | `(s: &[]u8): i64` | 通过 `parseUint` 解析输入数据，返回 `i64`。 |
| `parseMonth` | `(s: &[]u8): i32` | 通过 `parseMonth` 解析输入数据，返回 `i32`。 |
| `isLeapYear` | `(y: i64): bool` | 执行 `isLeapYear` 条件判定，返回 `bool`。 |
| `daysInMonth` | `(y: i64, m: i32): i32` | 调用 `daysInMonth` 并返回 `i32`。 |
| `daysFromCivil` | `(y_in: i64, m_in: i64, d: i64): i64` | 调用 `daysFromCivil` 并返回 `i64`。 |
| `parseHttpDate` | `(s: &[]u8): i64` | 通过 `parseHttpDate` 解析输入数据，返回 `i64`。 |
| `parseRetryAfter` | `(header: &[]u8): RetryAfter` | 通过 `parseRetryAfter` 解析输入数据，返回 `RetryAfter`。 |
| `waitSeconds` | `(ra: RetryAfter, now_epoch_sec: i64): i64` | 调用 `waitSeconds` 并返回 `i64`。 |
| `formatDelaySeconds` | `(seconds: u64, dst: []u8): i64` | 通过 `formatDelaySeconds` 编码或格式化输出，返回 `i64`。 |

## 最小调用示例

```cp
import { parseUint } from "retry-after";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseUint(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

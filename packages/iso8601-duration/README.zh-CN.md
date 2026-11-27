[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# iso8601-duration

Zero-allocation ISO 8601 duration parser, canonical formatter, and millisecond converter in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:iso8601-duration` ([https://github.com/tolu/ISO8601-duration](https://github.com/tolu/ISO8601-duration))（原库名 `iso8601-duration` → CP 包名 `iso8601-duration`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `iso8601-duration` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add iso8601-duration
```

```cp
import {
    IsoDuration,
    invalidDuration,
    parseIsoDuration,
    pushByte,
    pushU32,
    pushComp,
    formatIsoDuration,
    toApproxMillis,
} from "iso8601-duration";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `IsoDuration` | `class` | 字段: `negative: bool, years: u32, months: u32, weeks: u32, days: u32, hours: u32, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidDuration` | `(): IsoDuration` | 调用 `invalidDuration` 并返回 `IsoDuration`。 |
| `parseIsoDuration` | `(src: &[]u8): IsoDuration` | 通过 `parseIsoDuration` 解析输入数据，返回 `IsoDuration`。 |
| `pushByte` | `(dst: []u8, pos: usize, b: u8): i64` | 写入或更新状态数据。 |
| `pushU32` | `(dst: []u8, pos: usize, val: u32): i64` | 写入或更新状态数据。 |
| `pushComp` | `(dst: []u8, pos: usize, val: u32, unit: u8): i64` | 写入或更新状态数据。 |
| `formatIsoDuration` | `(d: IsoDuration, dst: []u8): i64` | 通过 `formatIsoDuration` 编码或格式化输出，返回 `i64`。 |
| `toApproxMillis` | `(d: IsoDuration): i64` | 调用 `toApproxMillis` 并返回 `i64`。 |

## 最小调用示例

```cp
import { parseIsoDuration } from "iso8601-duration";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = parseIsoDuration(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

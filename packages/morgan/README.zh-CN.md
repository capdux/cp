[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# morgan

Zero-heap HTTP access log template formatter supporting Morgan tiny/dev/short/common/combined presets and custom tokens (clean-room port of morgan v1.10)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:morgan` ([https://github.com/expressjs/morgan](https://github.com/expressjs/morgan))（原库名 `morgan` → CP 包名 `morgan`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `morgan` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add morgan
```

```cp
import {
    HttpLogEvent,
    mrgIsTokenChar,
    mrgWriteByte,
    mrgWriteOrDash,
    mrgWriteResponseTime,
    formatMorganLog,
    formatPreset,
} from "morgan";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HttpLogEvent` | `class` | 字段: `remote_addr: string, remote_user: string, date: string, method: string, url: string, http_version: string, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `mrgIsTokenChar` | `(c: u8): bool` | 调用 `mrgIsTokenChar` 并返回 `bool`。 |
| `mrgWriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `mrgWriteByte` 并返回 `usize`。 |
| `mrgWriteOrDash` | `(out: []u8, pos: usize, s: string): usize` | 调用 `mrgWriteOrDash` 并返回 `usize`。 |
| `mrgWriteResponseTime` | `(out: []u8, pos: usize, us: u64): usize` | 调用 `mrgWriteResponseTime` 并返回 `usize`。 |
| `formatMorganLog` | `(format_str: string, ev: HttpLogEvent, out: []u8): usize` | 通过 `formatMorganLog` 编码或格式化输出，返回 `usize`。 |
| `formatPreset` | `(preset: string, ev: HttpLogEvent, out: []u8): usize` | 通过 `formatPreset` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatMorganLog } from "morgan";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatMorganLog("example", /* ev: HttpLogEvent */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

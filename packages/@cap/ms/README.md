# `@cap/ms`

工业级、纯 CP 实现、零外部依赖、零堆分配的人类可读时长解析与格式化库，完整对标 npm [`ms`](https://www.npmjs.com/package/ms)（周下载 250M+）与 Rust [`humantime`](https://crates.io/crates/humantime) crate。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无 C/FFI、无 `unsafe`、无堆分配，全栈缓冲区操作，开箱即用。
- **双生态完整兼容**：
  - **npm `ms` 风格**：`parseMs` / `formatShort` / `formatLong`，支持纯数字毫秒、正负号、小数与全部大小写不敏感单位别名，以及短格式 / 自动单复数长格式输出。
  - **Rust `humantime` 风格**：`parseDurationNs` / `formatDurationNs`，支持多段复合纳秒级时长（如 `"1h 30m 15s 500ms"`、`"2d 4h"`、`"100us"`、`"500ns"`）与无损双向往返（round-trip）。
- **严格数值与边界防护**：全程纯整型 / 128 位定点数运算，零浮点漂移；内置溢出检测（`Overflow`）、空输入检测（`EmptyInput`）、非法格式拦截（`InvalidFormat`）、未知单位拦截（`UnknownUnit`）与输出缓冲区长度校验（`BufferTooSmall`）。

## 错误类型

```typescript
pub err MsError {
    EmptyInput,
    InvalidFormat,
    UnknownUnit,
    Overflow,
    BufferTooSmall,
}
```

| 错误码 | 说明 |
| :--- | :--- |
| `EmptyInput` | 输入为空字符串或仅包含空白字符 |
| `InvalidFormat` | 数值语法错误、缺少数字/单位、非法符号或多余片段 |
| `UnknownUnit` | 无法识别的时间单位别名 |
| `Overflow` | 数值计算或累加超出 `i64` / `u64` 范围 |
| `BufferTooSmall` | 提供的输出缓冲区 `out` 容量不足以容纳格式化结果 |

## 时间常量

| 常量 | 类型 | 值 | 说明 |
| :--- | :--- | ---: | :--- |
| `NS_PER_US` | `u64` | `1_000` | 1 微秒对应的纳秒数 |
| `NS_PER_MS` | `u64` | `1_000_000` | 1 毫秒对应的纳秒数 |
| `NS_PER_SEC` | `u64` | `1_000_000_000` | 1 秒对应的纳秒数 |
| `NS_PER_MIN` | `u64` | `60_000_000_000` | 1 分钟对应的纳秒数 |
| `NS_PER_HOUR` | `u64` | `3_600_000_000_000` | 1 小时对应的纳秒数 |
| `NS_PER_DAY` | `u64` | `86_400_000_000_000` | 1 天对应的纳秒数 |
| `NS_PER_WEEK` | `u64` | `604_800_000_000_000` | 1 周对应的纳秒数 |
| `MS_PER_SEC` | `i64` | `1_000` | 1 秒对应的毫秒数 |
| `MS_PER_MIN` | `i64` | `60_000` | 1 分钟对应的毫秒数 |
| `MS_PER_HOUR` | `i64` | `3_600_000` | 1 小时对应的毫秒数 |
| `MS_PER_DAY` | `i64` | `86_400_000` | 1 天对应的毫秒数 |
| `MS_PER_WEEK` | `i64` | `604_800_000` | 1 周对应的毫秒数 |
| `MS_PER_YEAR` | `i64` | `31_557_600_000` | 1 年对应的毫秒数（365.25 天） |

## API 文档

### `parseMs(input: []const u8): i64 ! MsError`

解析 npm `ms` 风格的时长字符串为有符号毫秒数（`i64`）：

- 支持纯数字毫秒（如 `"100"`, `"-50"`）；
- 支持正负号与小数（如 `"-3 days"`, `"2.5 hrs"`, `".5s"`, `"+1.5h"`）；
- 支持全部大小写不敏感单位别名：
  - 毫秒：`ms`, `msec`, `msecs`, `millisecond`, `milliseconds`
  - 秒：`s`, `sec`, `secs`, `second`, `seconds`
  - 分：`m`, `min`, `mins`, `minute`, `minutes`
  - 时：`h`, `hr`, `hrs`, `hour`, `hours`
  - 天：`d`, `day`, `days`
  - 周：`w`, `week`, `weeks`
  - 年：`y`, `yr`, `yrs`, `year`, `years`

### `parseDurationNs(input: []const u8): u64 ! MsError`

解析 Rust `humantime` 风格的多段复合非负时长为纳秒数（`u64`）：

- 支持空白分隔或紧凑拼接的多段时长（如 `"1h 30m 15s 500ms"`, `"2d 4h"`, `"1h30m"`, `"100us"`, `"500ns"`）；
- 额外支持 `ns`/`nsec`/`nsecs`/`nanosecond`/`nanoseconds` 与 `us`/`usec`/`usecs`/`microsecond`/`microseconds`/`µs`；
- 全程防整数溢出，超出 `u64` 范围时返回 `MsError.Overflow`。

### `formatShort(ms: i64, out: []u8): ([]const u8) ! MsError`

将有符号毫秒数格式化为 npm `ms` 短格式（四舍五入至最大适用单位，如 `"2d"`, `"10h"`, `"-3m"`, `"500ms"`），写入调用方提供的 `out` 缓冲区并返回实际写入的只读切片。

### `formatLong(ms: i64, out: []u8): ([]const u8) ! MsError`

将有符号毫秒数格式化为 npm `ms` 长格式并自动处理英文单复数（如 `"1 day"`, `"2 days"`, `"1 hour"`, `"-3 minutes"`, `"500 ms"`）。

### `formatDurationNs(ns: u64, out: []u8): ([]const u8) ! MsError`

将无符号纳秒数精确分解为 Rust `humantime` 风格的复合可读字符串（如 `"1h 30m 15s"`, `"0s"`, `"2d 3h 4m 5s 6ms"`），与 `parseDurationNs` 严格可逆。

## 使用示例

### 示例 1：npm `ms` 风格解析与短/长格式输出

```typescript
import {
    parseMs,
    formatShort,
    formatLong,
} from "@cap/ms";
import { print } from "cap:io";

pub fn main() {
    const ttl = parseMs("2.5 hrs") catch unreachable; // 9000000 ms
    const backoff = parseMs("-3 days") catch unreachable; // -259200000 ms
    const half_sec = parseMs(".5s") catch unreachable; // 500 ms

    let buf: [64]u8 = [0; 64];
    print(formatShort(ttl, buf[0..]) catch unreachable);      // "3h"
    print(formatLong(backoff, buf[0..]) catch unreachable);   // "-3 days"
    print(formatShort(half_sec, buf[0..]) catch unreachable); // "500ms"
}
```

### 示例 2：Rust `humantime` 风格多段纳秒时长解析与精确格式化

```typescript
import {
    parseDurationNs,
    formatDurationNs,
    NS_PER_HOUR,
    NS_PER_MIN,
    NS_PER_SEC,
} from "@cap/ms";
import { print } from "cap:io";

pub fn main() {
    const timeout_ns = parseDurationNs("1h 30m 15s 500ms") catch unreachable;
    const expected = NS_PER_HOUR + 30 * NS_PER_MIN + 15 * NS_PER_SEC + 500000000;
    if (timeout_ns != expected) {
        @panic("duration mismatch");
    }

    let buf: [128]u8 = [0; 128];
    const formatted = formatDurationNs(timeout_ns, buf[0..]) catch unreachable;
    print(formatted); // "1h 30m 15s 500ms"
}
```

### 示例 3：配置校验与错误处理（边界、溢出与缓冲区防护）

```typescript
import {
    MsError,
    parseMs,
    parseDurationNs,
    formatDurationNs,
} from "@cap/ms";
import { print } from "cap:io";

pub fn main() {
    // 1. 捕获未知单位错误
    * = parseMs("15 fortnight") catch e => {
        if (e == MsError.UnknownUnit) {
            print("rejected unknown duration unit");
        }
    };

    // 2. 捕获整数溢出错误
    * = parseDurationNs("999999999999999d") catch e => {
        if (e == MsError.Overflow) {
            print("rejected overflowing duration");
        }
    };

    // 3. 捕获输出缓冲区不足错误
    let tiny: [3]u8 = [0; 3];
    * = formatDurationNs(3661000000000, tiny[0..]) catch e => {
        if (e == MsError.BufferTooSmall) {
            print("buffer too small for formatted duration");
        }
    };
}
```

## 版权与许可

Copyright (c) 2026 CP Authors.

本项目为 Clean-Room 独立自主实现，遵循 **MIT License** 开源许可协议。

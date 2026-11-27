[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# parse-ms

纯 CP 实现的零堆分配时间跨度分解与重组库，支持毫秒、微秒、纳秒与秒级时长拆解（`days`/`hours`/`minutes`/`seconds`/`milliseconds`/`microseconds`/`nanoseconds`）及零堆格式化输出。

- **包名**：`parse-ms`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `npm/parse-ms`（`MIT`，`sindresorhus/parse-ms`），扩展了 `f64` 亚毫秒解析、纳秒/微秒构造、反向还原及格式化输出。

## 安装与导入

```bash
cap add parse-ms
```

```typescript
import {
    MS_PER_SECOND, MS_PER_MINUTE, MS_PER_HOUR, MS_PER_DAY,
    ParsedMs,
    parseMs, parseMsF64, parseMsF64Checked,
    fromNanoseconds, fromMicroseconds, fromSeconds,
    toMilliseconds, toMillisecondsF64, toNanoseconds,
    formatParsedMs,
} from "parse-ms";
```

## 核心 API

### 常量与 `ParsedMs` 类

| 符号 | 值 / 签名 | 说明 |
| --- | --- | --- |
| `MS_PER_SECOND` / `MS_PER_MINUTE` | `1000` / `60000`（`u64`） | 每秒 / 每分钟毫秒数。 |
| `MS_PER_HOUR` / `MS_PER_DAY` | `3600000` / `86400000`（`u64`） | 每小时 / 每天毫秒数。 |
| `ParsedMs` | 字段：`days`, `hours`, `minutes`, `seconds`, `milliseconds`, `microseconds`, `nanoseconds`（均为 `i64`）；方法：`zero()`, `isZero()`, `isNegative()`, `equals(other)` | 保存全部 7 个有符号时间分量的栈值结构体。 |

### 解析、构造、重组与格式化

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `parseMs` | `(ms: i64): ParsedMs` | 将整数毫秒（正/负）拆解为 `ParsedMs`（`microseconds = 0, nanoseconds = 0`）。 |
| `parseMsF64` | `(ms: f64): ParsedMs` | 拆解浮点毫秒，含小数部分的微秒与纳秒（NaN/Inf 返回零值）。 |
| `parseMsF64Checked` | `(ms: f64): ParsedMs?` | 同 `parseMsF64`，遇到 NaN 或无穷大返回 `null`。 |
| `fromNanoseconds` | `(ns_total: i64): ParsedMs` | 将总纳秒数拆解到全部 7 个字段。 |
| `fromMicroseconds` | `(us_total: i64): ParsedMs` | 将总微秒数拆解到 `days..microseconds`。 |
| `fromSeconds` | `(sec: i64): ParsedMs` | 将总秒数拆解到 `days..seconds`。 |
| `toMilliseconds` | `(p: ParsedMs): i64` | 重组为整数毫秒（忽略亚毫秒字段）。 |
| `toMillisecondsF64` | `(p: ParsedMs): f64` | 重组为浮点毫秒（包含微秒与纳秒）。 |
| `toNanoseconds` | `(p: ParsedMs): i64` | 从全部 7 个字段重组为总纳秒数。 |
| `formatParsedMs` | `(p: ParsedMs, out: []u8): usize` | 格式化为可读文本（如 `"1d 13h 56m 40s 999ms"`）写入 `out`，返回写入字节数。 |

## 使用示例

### 1. 解析毫秒并格式化

```typescript
import { parseMs, formatParsedMs } from "parse-ms";

fn main() {
    let p = parseMs(1337000001);
    // p.days == 15, p.hours == 11, p.minutes == 23, p.seconds == 20, p.milliseconds == 1
    let buf: [64]u8 = undefined;
    let n = formatParsedMs(p, buf[0..]);
}
```

### 2. 浮点亚毫秒解析（带 NaN/Inf 检查）

```typescript
import { parseMsF64Checked, toMillisecondsF64 } from "parse-ms";

fn main() {
    if (let p = parseMsF64Checked(12.345678)) {
        // p.milliseconds == 12, p.microseconds == 345, p.nanoseconds == 678
        let ms_back = toMillisecondsF64(p);
    }
}
```

### 3. 负时长与纳秒往返

```typescript
import { fromNanoseconds, toNanoseconds } from "parse-ms";

fn main() {
    let p = fromNanoseconds(-1500000500);
    let neg = p.isNegative();      // true
    let total = toNanoseconds(p);  // -1500000500
}
```

## 与源包差异及零拷贝安全亮点

- **全 64 位整数精度**：`parseMs(i64)` 避免 JS `Number` 53 位精度损失；`parseMsF64Checked` 通过 `ParsedMs?` 安全处理 NaN/Inf。
- **双向无损转换**：除拆解外，提供 `fromNanoseconds`/`fromMicroseconds`/`fromSeconds` 与 `toMilliseconds`/`toMillisecondsF64`/`toNanoseconds` 及 `formatParsedMs`。
- **零堆分配**：`ParsedMs` 为纯栈值类型（7 个 `i64` 字段），全程零堆分配。

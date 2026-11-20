# CP 标准库：人性化格式化与双向单位解析引擎 (`cap:intl` / `cap.intl`)

## 一、 架构总览

`cap:intl` 是 CP 语言官方内置的纯原生国际化数值格式化与双向单位解析标准库，专为 CLI 仪表盘、配置文件大小/时长解析（如 `"1.5 GiB"`、`"2h30m"`）与多地区数值/百分比/序数词/相对时间展示打造。全模块零 C 依赖、零 `std.posix`、零 `@import("bun")`，支持 `import { ... } from "cap:intl"` 与 `import cap from "cap"; cap.intl.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/intl/number.cp` | `// Source: cp/lib/intl/number.cp (Grouped, Compact, Ordinal & Percent Number Formatter)` | `IntlError`、`NumberFormatOptions`、`NumberFormatter`（千分位整数/浮点分组、紧凑 `K/M/B/T`、英文序数词与百分比格式化） |
| `lib/intl/units.cp` | `// Source: cp/lib/intl/units.cp (SI/IEC Byte Size, Human Duration & Relative Time Codec)` | `UnitError`、`ByteSize`、`HumanDuration`、`RelativeTime`（SI/IEC 字节大小与复合时长双向解析格式化、相对时间单复数描述） |
| `lib/intl/index.cp` | `// Source: cp/lib/intl/index.cp (Unified Exports for cap:intl)` | `cap:intl` 与 `cap.intl` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 国际化数值格式化（`number.cp`）

- **`IntlError`**：`InvalidOption`、`OutOfMemory`。
- **`NumberFormatOptions`**：
  - `thousands_sep: u8 = ','`：整数部分千分位分组分隔符（传 `0` 禁用分组）。
  - `decimal_sep: u8 = '.'`：小数点分隔符（欧式场景传 `','`）。
  - `decimals: u8 = 2`：保留小数位数（自动进位舍入）。
- **`NumberFormatter`**：
  - `formatInt(alloc, val: i64, sep: u8): ([]u8)!`：按 3 位分组格式化有符号整数（如 `1234567 -> "1,234,567"`，安全处理 `minInt(i64)`）。
  - `formatFloat(alloc, val: f64, opts: NumberFormatOptions): ([]u8)!`：支持美式 `1,234.56`、欧式 `1.234,56`、进位溢出修正（`99.996 -> "100.00"`）、`-0.0` 归零及 `NaN`/`Infinity`。
  - `formatCompact(alloc, val: f64): ([]u8)!`：紧凑计数格式化（`1200 -> "1.2K"`、`3400000 -> "3.4M"`、`5600000000 -> "5.6B"`、`7800000000000 -> "7.8T"`，自动剥离 `.0` 尾零并处理 `999.96K -> 1M` 升档）。
  - `formatOrdinal(alloc, n: u64): ([]u8)!`：精确英文序数词（`1st`、`2nd`、`3rd`、`4th`、`11th`、`12th`、`13th`、`21st`、`112th`）。
  - `formatPercent(alloc, ratio: f64, decimals: u8): ([]u8)!`：比率转百分比字符串（如 `0.256 -> "25.60%"`）。

### 2. 字节大小、复合时长与相对时间编解码（`units.cp`）

- **`UnitError`**：`InvalidFormat`、`Overflow`、`NegativeValue`、`OutOfMemory`。
- **`ByteSize`**：
  - `formatSi(alloc, bytes: u64): ([]u8) ! UnitError`：1000 进制格式化（`B` / `KB` / `MB` / `GB` / `TB` / `PB`，最多保留 2 位有效小数并自动去尾零）。
  - `formatIec(alloc, bytes: u64): ([]u8) ! UnitError`：1024 进制格式化（`B` / `KiB` / `MiB` / `GiB` / `TiB` / `PiB`）。
  - `parse(str: string): u64 ! UnitError`：大小写不敏感解析 `"1.5 GiB"`、`"256MB"`、`"512 k"`、`"1024"`，采用 `u128` 精确十进制乘法防溢出，负数触发 `UnitError.NegativeValue`，越界触发 `UnitError.Overflow`。
- **`HumanDuration`**：
  - `formatMs(alloc, ms: u64): ([]u8) ! UnitError`：将毫秒分解为复合人类可读时长（如 `"1d 2h 30m 15s"`、`"450ms"`、`"0ms"`）。
  - `parseMs(str: string): u64 ! UnitError`：解析复合或小数时长（如 `"1h30m15s"`、`"2d 4h"`、`"500ms"`、`"1.5h"`，支持 `ms`/`s`/`m`/`h`/`d`/`w` 及完整英文单复数），带负数与 `u64` 溢出校验。
- **`RelativeTime`**：
  - `formatDeltaSec(alloc, delta_sec: i64): ([]u8) ! UnitError`：按时间偏移量生成自然英文相对时间；`|delta| < 5` 返回 `"just now"`，负值为过去（`"1 minute ago"`、`"5 minutes ago"`、`"3 days ago"`），正值为将来（`"in 1 hour"`、`"in 2 hours"`），精确处理全部单位的单复数。

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    NumberFormatOptions,
    NumberFormatter,
    ByteSize,
    HumanDuration,
    RelativeTime,
} from "cap:intl";

pub fn demoDashboard()! {
    const alloc = std.heap.page_allocator;

    const users = NumberFormatter.formatInt(alloc, 1234567, ',')?;
    defer alloc.free(users); // "1,234,567"

    const euro = NumberFormatter.formatFloat(alloc, 1234.56, NumberFormatOptions {
        thousands_sep: '.',
        decimal_sep: ',',
        decimals: 2,
    })?;
    defer alloc.free(euro); // "1.234,56"

    const qps = NumberFormatter.formatCompact(alloc, 3400000.0)?;
    defer alloc.free(qps); // "3.4M"

    const rank = NumberFormatter.formatOrdinal(alloc, 21)?;
    defer alloc.free(rank); // "21st"

    const mem_limit = ByteSize.parse("1.5 GiB")?; // 1610612736
    const mem_label = ByteSize.formatIec(alloc, mem_limit)?;
    defer alloc.free(mem_label); // "1.5 GiB"

    const timeout_ms = HumanDuration.parseMs("2h 30m")?; // 9000000
    const uptime_str = cap.intl.HumanDuration.formatMs(alloc, 95415000)?;
    defer alloc.free(uptime_str); // "1d 2h 30m 15s"

    const synced = RelativeTime.formatDeltaSec(alloc, -300)?;
    defer alloc.free(synced); // "5 minutes ago"
}
```

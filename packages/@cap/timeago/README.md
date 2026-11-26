# @cap/timeago

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:timeago.js` ([https://github.com/hustcc/timeago.js](https://github.com/hustcc/timeago.js))（原库名 `timeago.js` → CP 包名 `@cap/timeago`）

Zero-allocation deterministic relative time formatter for CP — clean-room MIT parity with npm `timeago.js` / `date-fns/formatDistance` and Rust `chrono-humanize` / `timeago`.

零堆分配、确定性的 CP 相对时间格式化引擎，不依赖任何系统时钟调用，支持中英文（`En` / `Zh`）与长短紧凑多风格（`Long` / `Short` / `formatCompact`），精准处理单复数与过去/未来时间差（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/timeago
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `TimeagoLocale` | `pub enum TimeagoLocale: u8 { En, Zh }` | 语言枚举（英文 / 中文） |
| `TimeagoStyle` | `pub enum TimeagoStyle: u8 { Long, Short }` | 风格枚举（完整 / 简短） |
| `formatTimeago` | `(delta_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8) -> string` | 按有符号秒差格式化相对时间（负数=过去，正数=未来） |
| `formatBetween` | `(target_sec: i64, now_sec: i64, locale: TimeagoLocale, style: TimeagoStyle, out: []u8) -> string` | 计算 `target_sec - now_sec` 并格式化 |
| `formatTimeagoEn` | `(delta_sec: i64, out: []u8) -> string` | 英文 Long 快捷封装（如 `"5 minutes ago"`, `"in 3 hours"`, `"just now"`） |
| `formatTimeagoZh` | `(delta_sec: i64, out: []u8) -> string` | 中文 Long 快捷封装（如 `"5 分钟前"`, `"3 小时后"`, `"刚刚"`） |
| `formatCompact` | `(delta_sec: i64, out: []u8) -> string` | 监控/日志极简格式（`"now"`, `"-5m"`, `"+2h"`, `"-3d"`, `"-1y"`） |

## Usage / 使用示例

```cp
import {
    TimeagoLocale,
    TimeagoStyle,
    formatTimeago,
    formatBetween,
    formatTimeagoEn,
    formatTimeagoZh,
    formatCompact,
} from "@cap/timeago";

let buf: [64]u8 = undefined;
const en = formatTimeagoEn(-300, buf[0..64]);        // "5 minutes ago"
const zh = formatTimeagoZh(10800, buf[0..64]);       // "3 小时后"
const cli = formatCompact(-172800, buf[0..64]);      // "-2d"
const diff = formatBetween(1700000000, 1700003600, TimeagoLocale.En, TimeagoStyle.Short, buf[0..64]); // "1h ago"
```

## License

MIT

# @cap/human-number

Zero-allocation human-readable number formatting engine for CP — clean-room MIT parity with npm `numeral` / `millify` / `number-to-words` and Rust `num-format`.

零堆分配的 CP 人类可读数字格式化引擎，支持千分位分组、中英文紧凑缩写（`K/M/B/T`、`万/亿/万亿`）、百分比格式化、严格罗马数字双向转换（`1..3999`）与全范围英文数字拼写（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/human-number
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `formatGrouped` | `(n: i64, sep: u8, out: []u8) -> string` | 千分位分组格式化（支持负数与自定义分隔符，如 `1234567, ','` → `"1,234,567"`） |
| `formatCompactEn` | `(n: i64, out: []u8) -> string` | 英文紧凑缩写（`K`/`M`/`B`/`T`，如 `1200` → `"1.2K"`、`3500000` → `"3.5M"`） |
| `formatCompactZh` | `(n: i64, out: []u8) -> string` | 中文紧凑缩写（`万`/`亿`/`万亿`，如 `12500` → `"1.3万"`、`250000000` → `"2.5亿"`） |
| `formatPercent` | `(ratio: f64, decimals: u8, out: []u8) -> string` | 百分比格式化与四舍五入（如 `0.9995, 2` → `"99.95%"`） |
| `toRoman` | `(n: u16, out: []u8) -> string` | 整数转标准罗马数字（`1..=3999`，如 `2026` → `"MMXXVI"`） |
| `fromRoman` | `(s: string) -> u16?` | 严格解析罗马数字（大小写无关，拒绝 `"IIII"`/`"IC"`/`"VV"` 等非法形式） |
| `toEnglishWords` | `(n: i64, out: []u8) -> string` | 整数转英文拼写（支持 `0`、负数与完整 `i64` 范围） |

## Usage / 使用示例

```cp
import {
    formatGrouped,
    formatCompactEn,
    formatCompactZh,
    formatPercent,
    toRoman,
    fromRoman,
    toEnglishWords,
} from "@cap/human-number";

let buf: [256]u8 = undefined;

const g = formatGrouped(1234567, ',', buf[0..256]);       // => "1,234,567"
const en = formatCompactEn(2400000000, buf[0..256]);      // => "2.4B"
const zh = formatCompactZh(250000000, buf[0..256]);       // => "2.5亿"
const pct = formatPercent(0.9995, 2, buf[0..256]);        // => "99.95%"
const rom = toRoman(2026, buf[0..256]);                   // => "MMXXVI"
const num = fromRoman("MCMXCIV");                         // => 1994 (u16?)
const words = toEnglishWords(1234, buf[0..256]);          // => "one thousand two hundred thirty-four"
```

## License

MIT

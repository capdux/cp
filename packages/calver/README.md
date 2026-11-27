[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# calver

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:calver` ([https://github.com/muratgozel/node-calver](https://github.com/muratgozel/node-calver))

Zero-allocation Calendar Versioning (CalVer) parser, validator, comparator, bumper, and formatter for CP — clean-room MIT parity with npm `calver` and Rust `calver` (covering Ubuntu / Pip / Twisted / Unity / JetBrains / Black CalVer styles).

零堆分配的 CP 日历版本号（CalVer）解析、校验、比较、周期递增与格式化引擎，完整支持 2/4 位年份、零填充月份、日级与 micro 版本号、预发布修饰符（`-alpha.1`/`b2`/`-rc.1`）以及闰年日期合法性校验（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add calver
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `CalverFormat` | `pub enum { YyyyMmMicro, YyMmMicro, Yyyy0mMicro, YyyyMmDd, YyyyMmDdMicro }` | 5 种标准 CalVer 格式化预设 |
| `Calver` | `pub struct { valid, year, month, day, micro, modifier, modifier_num }` | 日历版本结构体 |
| `parseCalver` | `(raw: string) -> Calver` | 解析 CalVer 字符串（支持 2/4 位年份、可选日/micro 与预发布后缀） |
| `parseCalverWith` | `(raw: string, fmt: CalverFormat) -> Calver` | 按指定格式解析（用于区分 `YYYY.MM.DD` 与 `YYYY.MM.MICRO`） |
| `compareCalver` | `(a: Calver, b: Calver) -> i32` | 版本比较（`-1`/`0`/`1`，预发布版本低于正式版本） |
| `bumpCalver` | `(cur: Calver, year: u16, month: u8, day: u8, use_day: bool) -> Calver` | 同周期 `micro + 1`，新周期重置 `micro = 0` |
| `formatCalver` | `(v: Calver, fmt: CalverFormat, out: []u8) -> string` | 按指定格式输出日历版本号字符串 |

## Usage / 使用示例

```cp
import {
    CalverFormat,
    parseCalver,
    compareCalver,
    bumpCalver,
    formatCalver,
} from "calver";

let buf: [64]u8 = undefined;
const cur = parseCalver("2026.11.0");
const same_month = bumpCalver(cur, 2026, 11, 0, false); // 2026.11.1
const next_month = bumpCalver(cur, 2026, 12, 0, false); // 2026.12.0
const s = formatCalver(same_month, CalverFormat.YyyyMmMicro, buf[0..64]); // "2026.11.1"
```

## License

MIT

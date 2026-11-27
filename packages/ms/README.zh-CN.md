[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# ms

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`npm:ms`](https://github.com/vercel/ms)（MIT）| CP 包版本：`1.0.0` | 依赖：`itoa`

零堆分配的 CP 原生毫秒转换工具库。将人类可读的时长字符串（如 `"2 days"`、`"10h"`、`"2.5 hrs"`、`"-3 days"`）解析为 `i64` 毫秒，并将 `i64` 毫秒格式化为紧凑（`"1m"`、`"2d"`）或完整（`"1 minute"`、`"2 days"`）文本直接写入调用方缓冲区。

## 零成本迁移对照（TypeScript / JS → CP）

直接沿用原包名 `"ms"`，无需改名即可完成迁移：

| TypeScript / Node.js (`npm:ms`) | CP (`ms`) |
| :--- | :--- |
| `import ms from "ms";` | `import { parseMs, formatMs } from "ms";` |
| `ms("2 days")` → `172800000` | `parseMs("2 days").ms` → `172800000` |
| `ms(60000)` → `"1m"` | `formatMs(60000, false, out)` → `"1m"` |
| `ms(60000, { long: true })` → `"1 minute"` | `formatMs(60000, true, out)` → `"1 minute"` |

## 核心 API

### 导出常量

- `MS_SECOND: i64 = 1000`
- `MS_MINUTE: i64 = 60000`
- `MS_HOUR: i64 = 3600000`
- `MS_DAY: i64 = 86400000`
- `MS_WEEK: i64 = 604800000`
- `MS_YEAR: i64 = 31557600000`（365.25 天）

### 类型与函数

| 签名 | 说明 |
| :--- | :--- |
| `MsParseResult { ok: bool, ms: i64 }` | `parseMs` 返回值，读取 `ms` 前先检查 `ok`。 |
| `parseMs(input: string): MsParseResult` | 将时长字符串（最长 100 字节）解析为毫秒。 |
| `formatMs(ms: i64, long_fmt: bool, out: []u8): usize` | 将 `ms` 格式化写入 `out`（`long_fmt` 控制短/长格式），返回写入字节数。 |

## 实战示例

### 基础解析与格式化

```cp
import { parseMs, formatMs } from "ms";

fn demoBasic() {
    const r1 = parseMs("2 days");
    // r1.ok == true, r1.ms == 172800000

    const r2 = parseMs("2.5 hrs");
    // r2.ok == true, r2.ms == 9000000

    let buf: [32]u8 = undefined;
    const n_short = formatMs(60000, false, buf[0..]);
    // buf[0..n_short] == "1m"

    const n_long = formatMs(120000, true, buf[0..]);
    // buf[0..n_long] == "2 minutes"
}
```

### 边界场景与负数/非法输入

```cp
import { parseMs, formatMs, MS_DAY } from "ms";

fn demoEdgeCases() {
    // 负数时长与纯数字字符串
    const neg = parseMs("-3 days");
    // neg.ok == true, neg.ms == -3 * MS_DAY

    const bare = parseMs("100");
    // bare.ok == true, bare.ms == 100

    // 非法单位或超过 100 字节输入安全返回 ok=false
    const bad = parseMs("10 fortnights");
    // bad.ok == false, bad.ms == 0

    let buf: [32]u8 = undefined;
    const n = formatMs(-86400000, true, buf[0..]);
    // buf[0..n] == "-1 day"
}
```

## 性能与安全亮点

- **零堆分配**：格式化复用栈上缓冲区（`let tmp: [24]u8 = undefined`）与 `itoa::formatI64`，结果直写调用方 `out: []u8`。
- **`undefined` 零清零**：临时栈缓冲跳过无意义的初始化清零，降低热路径延迟。
- **无正则线性扫描**：手写单遍 ASCII 扫描器并内置 100 字符上限防护，免疫正则回溯风险。
- **所有权与越界安全**：所有输出写入均受切片长度约束，无缓冲区溢出与悬空指针风险。

## 许可协议

MIT

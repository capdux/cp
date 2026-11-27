[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# cronstrue

纯 CP 零堆分配 Cron 表达式中/英双语自然语言翻译引擎 —— 洁净室 MIT 实现，对标 npm `cronstrue`。

将标准 5 字段 POSIX Cron 表达式（`分 时 日 月 周`）及预定义宏（`@yearly`、`@annually`、`@monthly`、`@weekly`、`@daily`、`@midnight`、`@hourly`）直接翻译写入调用方栈缓冲区，支持英文（`Locale.En`）、中文（`Locale.Zh`）与 12/24 小时制切换。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源平台 | **npm** |
| 原包名 | `cronstrue`（CP 版 `v1.0.0`） |
| 上游仓库 | `https://github.com/bradymholt/cRonstrue` |
| 许可证 | MIT |

从 TypeScript/Node.js 迁移至 CP 时保持原包名不变：

```ts
// TypeScript / Node.js (npm: cronstrue)
import cronstrue from "cronstrue";
const text = cronstrue.toString("0 9 * * 1-5");
```

```cp
// CP (cap: cronstrue) — 零堆分配中/英双语翻译到栈缓冲
import { describeCron } from "cronstrue";
let buf: [128]u8 = undefined;
const text = describeCron("0 9 * * 1-5", buf[0..128]);
```

## 安装

```bash
cap add cronstrue
```

## 使用示例

### 基础用法：中英双语翻译与合法性校验

```cp
import {
    Locale,
    CronDescOptions,
    isValidCron,
    describeCron,
    describeCronWith,
} from "cronstrue";

fn demoDescribe() {
    let buf_en: [128]u8 = undefined;
    let buf_zh: [128]u8 = undefined;

    // 默认英文 24 小时制
    const en = describeCron("0 9 * * 1-5", buf_en[0..128]);
    // => "At 09:00, Monday through Friday"

    // 中文 24 小时制
    const zh = describeCronWith("0 9 * * 1-5", CronDescOptions.chinese(), buf_zh[0..128]);
    // => "在 09:00，周一至周五"

    const ok = isValidCron("*/15 * * * *"); // true
    _ = en;
    _ = zh;
    _ = ok;
}
```

### 边界用法：12 小时制、宏别名、英文月/周缩写与非法输入

```cp
import { Locale, CronDescOptions, describeCronWith, parseCron } from "cronstrue";

fn demoAdvanced() {
    let buf: [256]u8 = undefined;

    // 1. 切换 12 小时制
    let opts12 = CronDescOptions.english();
    opts12.use_24h = false;
    const t12 = describeCronWith("30 14 1 * *", opts12, buf[0..256]);
    // => "At 02:30 PM, on day 1 of the month"

    // 2. 支持 JAN-DEC / SUN-SAT 缩写（大小写不敏感）与预定义宏
    const named = describeCronWith("0 8 1,15 JAN,JUN MON-FRI", CronDescOptions.english(), buf[0..256]);
    const macro = describeCronWith("@weekly", CronDescOptions.chinese(), buf[0..256]);
    // => "在 00:00，仅在周日"

    // 3. 非法表达式安全返回空串 ""（绝不 Panic）
    const bad = describeCronWith("99 99 * * *", CronDescOptions.english(), buf[0..256]);
    // bad.len == 0

    // 4. 结构化 AST 解析
    const parsed = parseCron("@daily");
    // parsed.valid == true
    _ = t12;
    _ = named;
    _ = macro;
    _ = bad;
    _ = parsed;
}
```

## 翻译效果对照

| Cron 表达式 | 英文输出 (`Locale.En`, 24h) | 中文输出 (`Locale.Zh`, 24h) |
| --- | --- | --- |
| `* * * * *` | `Every minute` | `每分钟` |
| `*/15 * * * *` | `Every 15 minutes` | `每 15 分钟` |
| `0 9 * * 1-5` | `At 09:00, Monday through Friday` | `在 09:00，周一至周五` |
| `@daily` | `At 00:00, every day` | `每天 00:00` |
| `@hourly` | `Every hour` | `每小时` |

## 核心 API 速查

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `Locale` | `enum { En = 0, Zh = 1 }` | 输出语言枚举 |
| `CronDescOptions` | `class { locale: Locale, use_24h: bool }` | 翻译配置，提供 `english()` / `chinese()` 工厂方法 |
| `isValidCron` | `fn(expr: string): bool` | 严格校验 5 字段 Cron 表达式或预定义宏 |
| `describeCron` | `fn(expr: string, out: []u8): string` | 翻译为默认英文 24 小时制描述（非法返回 `""`） |
| `describeCronWith` | `fn(expr: string, opts: CronDescOptions, out: []u8): string` | 按自定义选项翻译（非法返回 `""`） |
| `parseCron` | `fn(raw: string): ParsedCron` | 解析为结构化字段 AST（非法时 `valid == false`） |
| `expandMacro` | `fn(trimmed: string): string` | 将 `@yearly`/`@daily`/`@hourly` 等宏展开为 5 字段串 |

## 性能与安全亮点

- **16-lane SIMD 加速**：复用 `cap:ascii`（`isSpace`、`isDigit`、`eqIgnoreCase`、`trim`）与 `cap:fmt`（`writeStr`、`writeU64`）加速词法扫描与格式化。
- **零堆分配栈池**：每个字段固定 `[16]FieldItem` 栈池，翻译结果直接写入调用方 `out: []u8` 缓冲区。
- **`undefined` 零清零优化**：内部切分边界数组采用 `undefined` 声明，消除无意义清零开销。
- **边界安全保护**：所有输出写入均受 `out.len` 保护，绝不越界。

## 许可证

MIT — 洁净室纯 CP 实现，灵感源自 npm `cronstrue`。

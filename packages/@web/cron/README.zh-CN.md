[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/cron

Official zero-heap 5-field Cron scheduler plugin for cap:web (bitmap matcher, pause/resume, status endpoint)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/cron` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/cron
```

```cp
import {
    CronTime,
    CronExpr,
    CronConfig,
    CronJob,
    CronScheduler,
    cronParseU8,
    cronParseField,
    cronWriteRaw,
} from "@web/cron";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CronTime` | `class` | 字段: `minute: u8, hour: u8, day: u8, month: u8, weekday: u8` |
| `CronExpr` | `class` | 字段: `minutes: u64, hours: u64, days: u64, months: u64, weekdays: u64` |
| `CronConfig` | `class` | 字段: `status_path: string` |
| `CronJob` | `class` | 字段: `used: bool, name_buf: [CRON_MAX_NAME_LEN]u8, name_len: usize, expr_buf: [CRON_MAX_EXPR_LEN]u8, expr_len: usize, expr: CronExpr, +3 项` · 方法: `nameSlice, exprSlice, matchesName` |
| `CronScheduler` | `class` | 字段: `jobs: [CRON_MAX_JOBS]CronJob, job_count: usize` · 方法: `init, reset, addJob, pauseJob, resumeJob, tick, +2 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cronParseU8` | `(s: string): u8?` | 调用 `cronParseU8` 并返回 `u8?`。 |
| `cronParseField` | `(field: string, min_val: u8, max_val: u8, is_weekday: bool): u64?` | 调用 `cronParseField` 并返回 `u64?`。 |
| `cronWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `cronWriteRaw` 并返回 `bool`。 |
| `cronWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `cronWriteU64` 并返回 `bool`。 |
| `parseCronExpr` | `(expr: string): CronExpr?` | 通过 `parseCronExpr` 解析输入数据，返回 `CronExpr?`。 |
| `cronMatches` | `(expr: &CronExpr, t: CronTime): bool` | 调用 `cronMatches` 并返回 `bool`。 |
| `cronGlobalScheduler` | `(): &mut CronScheduler` | 调用 `cronGlobalScheduler` 并返回 `&mut CronScheduler`。 |
| `resetCronScheduler` | `()` | 就地执行 `resetCronScheduler` 操作。 |
| `cronStatusHandler` | `(ctx: &mut Context): Response` | 调用 `cronStatusHandler` 并返回 `Response`。 |
| `createCronPlugin` | `(cfg: CronConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerCron` | `(app: &mut App, cfg: CronConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { parseCronExpr } from "@web/cron";

pub fn main() {
    let result = parseCronExpr("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

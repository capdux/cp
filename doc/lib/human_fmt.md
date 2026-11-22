# `cap:human_fmt` — 人类可读格式化、敏感 URL 脱敏与 CLI 表格渲染库

> 参考来源：/data/dev/bun/src/bun_core/fmt.cp | 吸收日期：2026-11-22

## 概述

`cap:human_fmt` 吸收自 Bun 核心 CLI 格式化工具集（`src/bun_core/fmt.cp`），提供纯 CP、零外部依赖、零堆分配的字节大小格式化、纳秒耗时自适应格式化、日志 URL 凭据/UUID/令牌自动脱敏以及 Unicode / ASCII 终端表格渲染。

## 上游特性与行为契约

- **Unicode / ASCII 双风格终端表格（吸收自 Bun `TableSymbols`）**：支持在彩色/TTY 终端使用 Unicode 盒图字符（`┌┬┐├┼┤└┴┘─│`），在非 TTY / CI 管道环境下无缝降级为纯 ASCII 边框（`|` / `-`），通过 `TableStyle` 一键切换。
- **日志防凭据泄漏脱敏（`redactUrl`）**：在打印 HTTP 请求错误、包下载日志或诊断堆栈前，自动对以下敏感片段执行就地脱敏：
  - URL 权威段中的密码（`https://user:secret@host` → 隐藏 `secret`）；
  - 36 字符标准 UUID（`xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx`）；
  - npm 访问令牌（`npm_*` 前缀）。
- **自适应量级缩放**：
  - `formatBytes`：自动按量级选择 `B` / `KB` / `MB` / `GB` / `TB`；
  - `formatDurationNs`：自动按耗时选择 `ns` / `us` / `ms` / `s` / `mXs`（分钟+秒）。

## 导出符号与 API

| 符号 | 说明 |
| :--- | :--- |
| `formatBytes(bytes, out)` | 自适应输出 `B` / `KB` / `MB` / `GB` / `TB`，缓冲不足返回 `null` |
| `formatDurationNs(ns, out)` | 自适应输出 `ns` / `us` / `ms` / `s` / `mXs`，缓冲不足返回 `null` |
| `redactUrl(input, out)` | 自动脱敏 URL 密码、UUID 与 `npm_*` 令牌 |
| `TableStyle` | 终端表格风格枚举（Unicode / ASCII） |
| `formatTableBorder` | 按指定列宽与 `TableStyle` 渲染表格上/中/下边框行 |
| `formatTableRow` | 按指定列宽与对齐规则渲染表格内容行 |

## 边界注意事项

1. **缓冲不足安全返回 `null`**：所有格式化与脱敏函数均不分配堆内存，目标缓冲 `out` 不足时安全返回 `null` 而不截断敏感数据。
2. **脱敏范围边界**：`redactUrl` 聚焦常见 URL 密码、UUID 与 `npm_*` 令牌；自定义业务 Header（如 `Authorization: Bearer ...`）应在日志层独立脱敏。

## 使用示例

```cp
import { formatBytes, formatDurationNs, redactUrl, TableStyle } from "cap:human_fmt";

fn demo() {
    let b1: [32]u8 = undefined;
    let b2: [32]u8 = undefined;
    let b3: [128]u8 = undefined;
    const sz = formatBytes(1536 * 1024, b1[0..]);
    const dur = formatDurationNs(2_500_000, b2[0..]); // "2.50ms"
    const safe = redactUrl("https://bot:s3cr3t@registry.npmjs.org/pkg?token=npm_abcdef1234", b3[0..]);
    * = sz; * = dur; * = safe; * = TableStyle;
}
```

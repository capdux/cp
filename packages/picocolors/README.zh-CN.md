[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `picocolors`

> **源包溯源**：`npm:picocolors` ([alexeyraspopov/picocolors](https://github.com/alexeyraspopov/picocolors)) — 协议：MIT（上游：ISC）

纯 CP 实现的零堆分配极速终端 ANSI 色彩与样式格式化库，支持嵌套关闭码自动恢复、`NO_COLOR` / `FORCE_COLOR` 环境探测、单次格式化器 `PicoFormatter` 以及多段拼接缓冲构建器 `PicoBuf`（基于 `ansi-styles`）。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `picocolors`，无需改名：

```typescript
// TypeScript / Node.js (npm: picocolors)
import pc from "picocolors";
console.log(pc.bold(pc.green("READY")) + " " + pc.gray("in 12ms"));
```

```cp
// CP（零堆分配，直接使用缓冲区或 PicoBuf 拼接）
import { PicoBuf, ColorConfig } from "picocolors";

let cfg = ColorConfig.fromEnv("", "", true); // NO_COLOR="", FORCE_COLOR="", is_tty=true
let pb = PicoBuf.fromConfig(cfg);
* = pb.green("READY");
* = pb.pushPlain(" ");
* = pb.gray("in 12ms");
let line = pb.buf[0..pb.len] as string;
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "picocolors": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 / 成员 | 说明 |
| :--- | :--- | :--- |
| `detectColorSupport` | `(no_color: string, force_color: string, is_tty: bool): bool` | 按标准 `NO_COLOR` / `FORCE_COLOR` / TTY 规则检测颜色支持 |
| `ColorConfig` | `class { is_color_supported: bool }` — `enabled()`、`disabled()`、`fromEnv(no_color, force_color, is_tty)` | 终端色彩开关配置 |
| `PicoFormatter` | `class { cfg: ColorConfig }` — `new(bool)`、`fromConfig(cfg)`、`isColorSupported()`、`style(pair, text, out)` + 25 种样式/颜色方法 | 将单段带样式文本写入 `out: []u8`，返回写入字节数 |
| `PicoBuf` | `class { cfg: ColorConfig, buf: [1024]u8, len: usize }` — `new(bool)`、`fromConfig(cfg)`、`clear()`、`pushPlain(text)`、`pushStyled(pair, text)` + 19 种常用样式追加方法 | 在 1024 字节栈缓冲中连续拼接多段彩色日志 |
| `picoFormatPair` / `picoMatchAt` | 底层辅助函数 | 直接按 `StylePair` 格式化并处理嵌套关闭码恢复 |

### 支持的样式与颜色

- **修饰符（`PicoFormatter`）**：`reset`、`bold`、`dim`、`italic`、`underline`、`inverse`、`hidden`、`strikethrough`
- **前景色**：`black`、`red`、`green`、`yellow`、`blue`、`magenta`、`cyan`、`white`、`gray`
- **背景色**：`bgBlack`、`bgRed`、`bgGreen`、`bgYellow`、`bgBlue`、`bgMagenta`、`bgCyan`、`bgWhite`
- **`PicoBuf` 快捷方法**：`bold`、`dim`、`italic`、`underline`、`inverse`、`strikethrough`、`black`、`red`、`green`、`yellow`、`blue`、`magenta`、`cyan`、`white`、`gray`、`bgRed`、`bgGreen`、`bgYellow`、`bgBlue`（其余可通过 `pushStyled(pair, text)` 调用）

## 实战示例

### 1. 使用 `PicoFormatter` 格式化并支持嵌套样式恢复

```cp
import { PicoFormatter } from "picocolors";

let pc = PicoFormatter.new(true);

let inner_buf: [64]u8 = undefined;
let n_inner = pc.bold("WARN", inner_buf[0..64]);
let inner = inner_buf[0..n_inner] as string;

// 外层黄色包裹内层加粗：内嵌关闭码会被自动恢复为外层开启码
let out_buf: [128]u8 = undefined;
let n_outer = pc.yellow(inner, out_buf[0..128]);
let styled = out_buf[0..n_outer] as string;
```

### 2. 使用 `PicoBuf` 构建多色 CLI 日志行

```cp
import { ColorConfig, PicoBuf } from "picocolors";

let cfg = ColorConfig.fromEnv("", "1", false); // FORCE_COLOR="1" 在非 TTY 下也启用色彩
let pb = PicoBuf.fromConfig(cfg);

* = pb.bgGreen(" PASS ");
* = pb.pushPlain(" ");
* = pb.bold("test_parser.cp");
* = pb.pushPlain(" ");
* = pb.dim("(4ms)");

let output = pb.buf[0..pb.len] as string;
pb.clear(); // 清空缓冲区以复用
```

## 注意事项与边界说明

- **环境变量优先级**：`NO_COLOR` 非空时禁用；`FORCE_COLOR="0"` 禁用、其余非空值强制启用；均为空时回退到 `is_tty`。
- **无色降级**：当 `is_color_supported == false` 时，所有方法均只输出纯文本，不注入任何 ANSI 转义序列。
- **缓冲区上限**：`PicoBuf.buf` 固定为 `[1024]u8`，每行输出后调用 `pb.clear()` 复用。

[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `chalk`

面向 CP 语言的零分配终端文本着色与样式库——完整对标 **npm `chalk` v5**，内置终端色彩能力自动检测（`supports-color`）、Truecolor (16M) / ANSI 256 / ANSI 16 / 纯文本自动降级，以及前景色、背景色与文本修饰组合。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:chalk` ([chalk/chalk](https://github.com/chalk/chalk), v5 对齐) |
| **CP 包名与版本** | `chalk@1.0.0` (`MIT`) |
| **依赖** | `ansi-styles@1.0.0`、`supports-color@1.0.0` |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm chalk)
import chalk from "chalk";
console.log(chalk.red("Error!") + " " + chalk.hex("#FF8800")("Warn"));
```

```cp
// CP — 保持原包名，零堆分配调用方缓冲
import { Chalk } from "chalk";

const c = Chalk.new(3); // Level 3 = Truecolor (16M)
let buf: [128]u8 = undefined;
const n1 = c.red("Error!", buf[0..]);
const n2 = c.hex("#FF8800", "Warn", buf[0..]);
```

## 架构特性与零堆设计

- **零堆分配**：所有样式方法均将 ANSI SGR 转义序列直接写入调用方栈缓冲 `out: []u8`，返回精确写入字节数。
- **自动色彩等级降级**：
  - Level `0`（`COLOR_LEVEL_NONE`）：纯文本直通，不输出任何 ANSI 转义码
  - Level `1`（`COLOR_LEVEL_BASIC`）：16 色基础 SGR（RGB / Hex / 关键字自动降级）
  - Level `2`（`COLOR_LEVEL_256`）：256 色 SGR（`38;5;n` / `48;5;n`）
  - Level `3`（`COLOR_LEVEL_16M`）：24 位 Truecolor（`38;2;r;g;b` / `48;2;r;g;b`）

## 实战示例

### 1. 基础前景色、背景色与文本修饰

```cp
import { Chalk } from "chalk";

fn demoBasic() {
    const c = Chalk.new(3);
    let buf: [256]u8 = undefined;

    const n1 = c.green("PASS", buf[0..]);
    const n2 = c.bgRed("FAIL", buf[0..]);
    const n3 = c.bold("Heading", buf[0..]);
    const n4 = c.underline("https://cp-lang.org", buf[0..]);
    const n5 = c.dim("debug trace", buf[0..]);
    _ = n1; _ = n2; _ = n3; _ = n4; _ = n5;
}
```

### 2. Truecolor RGB、Hex、CSS 关键字与自动降级

```cp
import { Chalk } from "chalk";

fn demoColors() {
    let buf: [256]u8 = undefined;

    // Level 3 输出 24 位 Truecolor (\x1b[38;2;255;136;0m...)
    const tc = Chalk.new(3);
    const n_rgb = tc.rgb(255, 136, 0, "orange", buf[0..]);
    const n_hex = tc.hex("#00D4FF", "cyan-glow", buf[0..]);
    const n_kw = tc.keyword("rebeccapurple", "purple", buf[0..]);
    const n_256 = tc.ansi256(208, "xterm-208", buf[0..]);

    // Level 0 自动剥离样式（适合 CI 日志或管道重定向输出）
    const plain = Chalk.new(0);
    const n_plain = plain.hex("#00D4FF", "cyan-glow", buf[0..]); // "cyan-glow"
    _ = n_rgb; _ = n_hex; _ = n_kw; _ = n_256; _ = n_plain;
}
```

### 3. 终端环境检测（`Chalk.fromEnv`）与组合样式（`styled`）

```cp
import { Chalk } from "chalk";
import { TerminalEnv } from "supports-color";
import { FG_CYAN } from "ansi-styles";

fn demoEnvAndStyled(env: TerminalEnv, argv: &[]string) {
    // 自动识别 NO_COLOR、FORCE_COLOR、COLORTERM、TERM 及 --color / --no-color 参数
    const c = Chalk.fromEnv(env, argv);

    // 单次零堆写入组合粗体 + 下划线 + 前景色
    let buf: [256]u8 = undefined;
    const n = c.styled("Deployment Ready", true, true, FG_CYAN, buf[0..]);
    _ = n;
}
```

## 核心 API 速览

- **构造器**：`Chalk.new(level: u8)`、`Chalk.fromEnv(env: TerminalEnv, argv: &[]string)`
- **前景色**：`black`、`red`、`green`、`yellow`、`blue`、`magenta`、`cyan`、`white`、`gray`
- **背景色**：`bgBlack`、`bgRed`、`bgGreen`、`bgYellow`、`bgBlue`、`bgMagenta`、`bgCyan`、`bgWhite`、`bgGray`
- **文本修饰**：`bold`、`dim`、`italic`、`underline`、`inverse`、`strikethrough`
- **扩展色彩**：`ansi256`、`bgAnsi256`、`rgb`、`bgRgb`、`hex`、`bgHex`、`keyword`、`bgKeyword`
- **组合样式**：`styled(text, is_bold, is_underline, fg, out)`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。

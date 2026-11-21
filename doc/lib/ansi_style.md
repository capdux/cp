# cap:ansi_style

> 源码吸收自 `/data/dev/bun/src/bun_core/output.cp`，提供纯 CP、零外部依赖、零堆分配的 ANSI 终端颜色/样式格式化、CSI/OSC 转义剥离与可见宽度统计工具库。

## 导入方式

```cp
import { AnsiColor, AnsiStyle, stripAnsi, visibleLen, hasAnsi } from "cap:ansi_style";
```

## 核心类型与 API

### `AnsiColor`（标准 16 色）

`black`、`red`、`green`、`yellow`、`blue`、`magenta`、`cyan`、`white` 及对应 `bright_*`（共 16 色）。
- `fgCode(): u8`：前景色 SGR 代码（`30..37` / `90..97`）。
- `bgCode(): u8`：背景色 SGR 代码（`40..47` / `100..107`）。

### `AnsiStyle`

字段：`fg: AnsiColor?`、`bg: AnsiColor?`、`bold: bool`、`dim: bool`、`italic: bool`、`underline: bool`、`strikethrough: bool`。

- `isEmpty(): bool`：是否未启用任何颜色或修饰。
- `format(text: []const u8, out: []u8): ([]const u8)?`：生成 `\x1b[...m<text>\x1b[0m`；空样式直接返回原文本切片。
- `AnsiStyle.formatRgb(r: u8, g: u8, b: u8, text: []const u8, out: []u8): ([]const u8)?`：生成 24-bit TrueColor `\x1b[38;2;<r>;<g>;<b>m<text>\x1b[0m`。

### 工具函数

- `stripAnsi(input: []const u8, out: []u8): ([]const u8)?`：剥离 CSI（`\x1b[...m`）与 OSC（`\x1b]...(\x07|\x1b\\)`，含 OSC 8 超链接）序列。
- `visibleLen(input: []const u8): usize`：统计剔除 ANSI 控制序列后的纯可见字节长度。
- `hasAnsi(input: []const u8): bool`：检测字符串是否包含 ANSI 控制序列。

## 使用示例

```cp
import { AnsiColor, AnsiStyle, stripAnsi, visibleLen } from "cap:ansi_style";

fn example() {
    let buf: [128]u8 = undefined;
    const style = AnsiStyle{ fg: AnsiColor.red, bold: true, underline: true };
    if (styled := style.format("ERROR", &buf)) {
        * = visibleLen(styled); // 5
        let plain_buf: [64]u8 = undefined;
        * = stripAnsi(styled, &plain_buf); // "ERROR"
    }
}
```

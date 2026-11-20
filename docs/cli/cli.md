# CP 标准库：命令行解析、POSIX 终端探测、16/256/RGB 配色、Spinner 与多边框表格手册 (`cap:cli`)

> **上游溯源**: `/data/dev/bun/src/clap/args.cp`, `/data/dev/bun/src/bun_core/tty.cp`, `/data/dev/bun/src/cli/cli.cp`  
> **模块路径**: `lib/cli/`  
> **导入协议**: `import { CliArgs, SliceIterator, TtyMode, Terminal, AnsiColor, Spinner, SpinnerStyle, ProgressBar, CliTable, CliTableAlign, CliTableBorder } from "cap:cli";`

---

## 一、 架构总览与核心能力

`cap:cli` 提供零堆分配的完整终端交互工具链：

1. **命令行参数解析 (`CliArgs`, `SliceIterator`)**：支持 `--flag`、`--key=value`、组合短标志 `-vf`、`-p=8080` 与 `--` 位置分隔符。
2. **真实 POSIX 终端探测 (`Terminal`, `TtyMode`)**：
   - `Terminal.isTty(fd)` 调用 `std.c.isatty(fd)` 探测交互式终端；
   - `Terminal.getSize(fd)` 通过 Linux `ioctl(TIOCGWINSZ)` 获取真实终端行列尺寸，非 TTY 或失败时安全回退为 `80x24`；
   - 提供 `supportsColor(fd)`（尊重 `NO_COLOR` 与 `TERM=dumb`）与 `supportsTrueColor()`（检测 `COLORTERM`）。
3. **完整 ANSI 16/256/24-bit TrueColor 与转义剥离 (`AnsiColor`)**：
   - 覆盖 16 标准前景色/背景色、文本样式（`BOLD`, `DIM`, `ITALIC`, `UNDERLINE`, `BLINK`, `INVERSE`, `HIDDEN`, `STRIKETHROUGH`）；
   - 提供零堆分配 `fg256` / `bg256`（`\x1b[38;5;Nm`）与 `fgRgb` / `bgRgb`（`\x1b[38;2;R;G;Bm`）；
   - `stripAnsi` 完整剥离 CSI（含 256 色/TrueColor）、OSC（`\x1b]...BEL/ST`）及 2 字节 ESC 控制序列。
4. **动画指示器 (`Spinner`) 与吞吐/ETA 进度条 (`ProgressBar`)**：
   - `Spinner` 支持 `Dots` / `Line` / `Arrow` 多帧动画推进（`tick`）、成功/失败终态切换（`succeed`/`fail`）及文本渲染；
   - `ProgressBar` 在 `[#####-----]` 条形渲染基础上，新增 `itemsPerSec()` 吞吐计算、`etaSeconds()` 剩余时间估算与 `renderLine()` 完整状态行渲染。
5. **Unicode 宽字符对齐表格 (`CliTable`)**：
   - 集成 `cap:string` 的 `stringWidth` 精确计算中文 CJK（双宽）、Emoji 与含 ANSI 转义字符串的显示列宽；
   - 支持 `CliTableBorder.Ascii`（`+---+`）、`CliTableBorder.Box`（`┌─┬─┐`）与 `CliTableBorder.Markdown`（`| :--- | :---: | ---: |`）三种边框风格。

---

## 二、 实战代码示例

### 1. 终端尺寸探测、TrueColor 与 ANSI 剥离

```typescript
import { Terminal, AnsiColor } from "cap:cli";

pub fn demoTerminalAndAnsi() {
    const sz = Terminal.getSize(1); // 真实 TTY 返回实际行列，管道/非 TTY 安全返回 80x24
    let rgb_buf: [32]u8 = undefined;
    const orange = AnsiColor.fgRgb(255, 128, 0, &rgb_buf);

    let plain_buf: [128]u8 = undefined;
    const clean = AnsiColor.stripAnsi("\x1b[38;2;255;0;0mFAIL\x1b[0m", &plain_buf); // "FAIL"
}
```

### 2. `Spinner`、`ProgressBar` 与 CJK/Box/Markdown `CliTable`

```typescript
import { Spinner, SpinnerStyle, ProgressBar, CliTable, CliTableAlign, CliTableBorder } from "cap:cli";

pub fn demoWidgets() {
    let sp = Spinner.init("Building", SpinnerStyle.Dots);
    * = sp.tick();
    sp.succeed("Done");

    let pb = ProgressBar.init(100, 10);
    pb.tick(50);
    let line_buf: [128]u8 = undefined;
    const line = pb.renderLine(&line_buf);

    let tbl = CliTable.init(3);
    tbl.setBorder(CliTableBorder.Box);
    tbl.setHeader(0, "模块", CliTableAlign.Left);
    tbl.setHeader(1, "状态", CliTableAlign.Center);
    tbl.setHeader(2, "耗时", CliTableAlign.Right);
    * = tbl.addRow3("测试", "🚀", "12ms");

    let out: [512]u8 = undefined;
    const len = tbl.renderToBuf(&out);
}
```

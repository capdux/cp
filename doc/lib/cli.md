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

---

### 3. CI/CD 环境探测 (`CiInfo` / `CiVendor`)、多级子命令路由 (`SubcommandRouter`) 与并发进度树 (`MultiProgress`)

源自 `/data/dev/bun/src/cli/ci_info.cp`、`/data/dev/bun/src/cli/cli.cp` 与 `/data/dev/bun/src/bun_core/Progress.cp`：

- **`CiInfo` & `CiVendor`** (`lib/cli/ci_info.cp`)：
  - 探测 GitHub Actions、GitLab CI、CircleCI、Travis CI、Jenkins、Buildkite、Azure Pipelines、Vercel、Netlify、Cloudflare Pages、Bitbucket Pipelines、TeamCity、AWS CodeBuild、Drone 与 Generic CI；
  - 自动解析 Pull Request 状态（`is_pr`）、ANSI/TrueColor/OSC-8 超链接支持能力（尊重 `NO_COLOR`、`FORCE_COLOR`、`TERM=dumb`）以及 Linux `ioctl(TIOCGWINSZ)` 真实终端宽高（兼容 `COLUMNS` 覆盖）；
  - 提供 `CiInfo.detect()`、`CiInfo.detectWithEnv(...)` 与 Bun 兼容函数 `isCI()` / `detectCIName()`。
- **`SubcommandRouter` & `SubcommandSpec`** (`lib/cli/subcommand.cp`)：
  - 支持多级子命令路由（如 `pm cache ls`）、别名匹配、全局/命令标志定义；
  - 基于 Levenshtein 编辑距离提供 `didYouMean(input, parent)` 拼写纠错建议；
  - `formatHelp()` 自动计算对齐列宽并生成彩色 `--help` 文本。
- **`MultiProgress`** (`lib/cli/subcommand.cp`)：
  - 基于原子自旋锁的线程安全多任务进度树，支持父子任务层级、帧动画 `Spinner`、吞吐格式化（`MB/s`、`files/s`）、`ETA` 计算与树状渲染（`renderTree`）。

---

### 4. 零分配扩展渲染与探测子模块（收敛自原 `cli_*` 系列）

- **色彩能力探测 (`lib/cli/color.cp`)**：
  - `ColorLevel`、`ColorEnv`、`parseForceColor`、`detectColorLevel`、`supportsColor`、`supportsTruecolor`（尊重 `NO_COLOR`、`FORCE_COLOR`、`COLORTERM`、`TERM`、`CI` 等环境信号）。
- **确定性进度计量器 (`lib/cli/progress.cp`)**：
  - `ProgressMeter`（`init`/`set`/`inc`/`percent`/`isComplete`/`etaMs`/`render`）、`ProgressSpinnerStyle`、`spinnerFrame`。
- **加载动画帧序列渲染器 (`lib/cli/spinner.cp`)**：
  - `SpinnerPreset`（`dots`/`line`/`arc`/`arrows`/`bouncing_bar`）、`SpinnerSpec`（`forPreset`/`frameAt`/`frameAtMs`）、`formatSpinnerFrame`、`formatSpinnerDone`。
- **零分配表格边框与列宽渲染器 (`lib/cli/table_render.cp`)**：
  - `TableBorderStyle`（`ascii`/`compact`/`unicode_light`）、`CellAlign`（`left`/`right`/`center`）、`computeColumnWidths`、`renderBorderLine`、`renderTableRow`、`renderTable`。
- **零分配树形 / 依赖树渲染器 (`lib/cli/tree.cp`)**：
  - `TreeStyle`（`unicode`/`ascii`/`rounded`）、`TreePrefix`（64 层祖先位掩码）、`TreeNode`、`renderFlatTree`。

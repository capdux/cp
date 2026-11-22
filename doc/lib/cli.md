# CP 标准库：命令行解析、POSIX 终端探测、16/256/RGB 配色、Spinner 与多边框表格手册 (`cap:cli`)

> 参考来源：`/data/dev/bun/src/clap/args.cp`, `/data/dev/bun/src/bun_core/tty.cp`, `/data/dev/bun/src/cli/cli.cp` | 吸收日期：2026-11-22
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
- **零分配冒号/等号键值列表解析器 (`lib/cli/colon_list.cp`，吸收自 `/data/dev/bun/src/cli/colon_list_type.cp`)**：
  - 错误集与结构体：`ColonListError`（`InvalidSeparator` / `EmptyKey` / `InvalidExtensionPrefix` / `OutputBufferTooSmall`）、`ColonPair`
  - 解析与查询函数：`parseColonPair`、`parseColonPairNonEmptyKey`、`parseLoaderMapping`、`parseColonList`、`parseLoaderMappingList`、`findColonValue`（优先匹配先出现的 `:` 或 `=`，兼容 Bun/esbuild CLI `--define`/`--loader` 语法，支持 last-wins 查询）

```typescript
import { ColonPair, parseLoaderMappingList, findColonValue } from "cap:cli";

const raw: [2]([]const u8) = [".ts:tsx", ".svg=text"];
let pairs: [2]ColonPair = [ColonPair{}, ColonPair{}];
const n = parseLoaderMappingList(raw[0..], pairs[0..])?;
const svg_loader = findColonValue(pairs[0..n], ".svg") ?? ""; // "text"
```
- **零分配 Shell 补全探测与格式化器 (`lib/cli/completions.cp`，吸收自 `/data/dev/bun/src/cli/shell_completions.cp`)**：
  - 类型与函数：`CompletionError` (`BufferTooSmall`)、`CompletionShell`（`unknown` / `bash` / `zsh` / `fish` / `pwsh`，含 `fromShellPath` / `name` / `delimiter`）、`CompletionItem`、`formatCompletions`、`formatCompletionItems`。

```typescript
import { CompletionShell, CompletionItem, formatCompletionItems } from "cap:cli";

const sh = CompletionShell.fromShellPath("/usr/bin/zsh"); // CompletionShell.zsh
const items: [2]CompletionItem = [
    CompletionItem{ command: "build", description: "Build project" },
    CompletionItem{ command: "bench", description: "Run benchmarks" },
];
let buf: [128]u8 = [0; 128];
const n = formatCompletionItems(sh, items[0..], "bu", buf[0..])?; // "build\tBuild project"
```
- **Yarn v1/v2 命令分类与包管理进度标签渲染 (`lib/cli/pm_commands.cp`，吸收自 `/data/dev/bun/src/cli/list-of-yarn-commands.cp` & `src/install/PackageManager/ProgressStrings.cp`)**：
  - 类型与函数：`PmCommandsError` (`BufferTooSmall`)、`YarnCommandEra`（`none`/`v1_only`/`v2_only`/`both`）、`PmStage`（`resolve`/`extract`/`install`/`save_lockfile`/`run_script`）、`yarnCommandEra`、`isYarnCommand`、`mapYarnToCapSubcommand`、`pmStagePlainName`、`pmStageLabel`、`formatPmStageBanner`。

```typescript
import { PmStage, yarnCommandEra, mapYarnToCapSubcommand, formatPmStageBanner } from "cap:cli";

const era = yarnCommandEra("dlx"); // YarnCommandEra.v2_only
const mapped = mapYarnToCapSubcommand("up") ?? ""; // "update"
let buf: [128]u8 = [0; 128];
const banner = formatPmStageBanner(buf[0..], PmStage.install, "lodash@4.17.21", false)?;
// "Installing lodash@4.17.21"
* = era;
* = mapped;
* = banner;
```
- **零分配 `bun info` / `bun pm view` 包元数据查询与卡片渲染 (`lib/cli/pm_view.cp`，吸收自 `/data/dev/bun/src/cli/pm_view_command.cp`)**：
  - 常量与类型：`PM_VIEW_MAX_PATH_SEGMENTS` (`16`)、`PM_VIEW_RECENT_VERSIONS_LIMIT` (`5`)、`PmViewError`、`PmViewSpec`（`parse` / `isScoped` / `buildRegistryUrl`，自动编码作用域 `/` 为 `%2F`）、`PmPropertyPathSegment`、`PmPropertyPath`（支持 `repository.url`、`maintainers[0].email`、`keywords.0` 点分与数组下标查询）、`PmViewDistInfo`、`PmViewKv`、`PmViewMaintainer`、`PmViewSummary`（`formatCard`）、`formatRecentVersionsHint`、`formatPmViewJsonError`。

```typescript
import { PmViewSpec, PmPropertyPath } from "cap:cli";

const spec = PmViewSpec.parse("@types/react@18.2.15")?;
let url_buf: [128]u8 = [0; 128];
const n = spec.buildRegistryUrl("https://registry.npmjs.org/", url_buf[0..])?;
// url_buf[0..n] == "https://registry.npmjs.org/@types%2Freact"
const prop = PmPropertyPath.parse("maintainers[0].email")?;
// prop.count() == 3 ("maintainers", [0], "email")
* = n;
* = prop;
```

---

## 上游 Bun 行为契约与终端边界注意事项

1. **色彩降级与环境变量优先级（`lib/cli/color.cp` & `lib/cli/pm_commands.cp`）**：
   - `detectColorLevel` 严格遵循标准终端环境变量契约：`NO_COLOR` 非空或 `TERM=dumb` 时强制关闭色彩；`FORCE_COLOR`（`0`/`1`/`2`/`3`）优先覆盖非 TTY 重定向探测；`COLORTERM=truecolor|24bit` 启用 24-bit TrueColor；在 CI 环境（`lib/cli/ci_info.cp`）中自动识别非交互流水线，并通过 `pmStageLabel(stage, false)` / `formatPmStageBanner(..., false)` 自动降级为无 Emoji 的纯文本进度输出（配合 `/data/dev/bun/docs/pm/cli/pm.mdx` 的 `--quiet` 脚本静默输出规范）。
2. **参数解析 `--` 终止哨兵（`CliArgs`）**：
   - 遇到独立 `--` 标志后，其后所有参数无论是否以 `-` 开头均按位置参数透传，与 POSIX `getopt` 及 Bun CLI 参数规范完全一致。
3. **非 TTY 与管道重定向安全回退**：
   - 当标准输出被重定向至文件或管道时，`Terminal.isTty(fd)` 返回 `false`，`Terminal.getSize(fd)` 安全回退至 `80x24`，避免 `ioctl(TIOCGWINSZ)` 失败引发异常。
4. **宽字符与 ANSI 转义宽度剥离**：
   - `CliTable` 与 `renderTable` 在计算列宽前先剥离 ANSI 色彩转义序列（`AnsiColor.stripAnsi`），再按 Unicode 显示宽度对齐 CJK 双宽字符，避免彩色表格边框错位。
5. **`bun info` / `bun pm view` 元数据查询与别名契约（吸收自 `/data/dev/bun/docs/pm/cli/info.mdx`）**：
   - `bun pm view` 为 `bun info` 的等价别名；未指定版本时默认查询 `latest`，支持 `<pkg>@<version>` 精确版本或范围查询，并可通过点分/数组下标路径（如 `version`、`dependencies`、`repository.url`、`maintainers[0].email`）直接提取单一字段；
   - 面向脚本消费时推荐使用 `--json` 输出结构化 JSON（缺失版本或缺失属性时通过 `formatPmViewJsonError` 输出带转义的标准错误 JSON），终端交互模式下缺失版本时自动展示最近 5 个可用版本提示（`formatRecentVersionsHint`）。
6. **编辑器自动探测与行/列跳转命令生成（`lib/cli/open.cp`，吸收自 `/data/dev/bun/src/cli/open.cp` 与 `/data/dev/bun/docs/runtime/utils.mdx`）**：
   - **探测优先级**：先采用显式配置（如 `bunfig.toml` 的 `[debug] editor = "code"` 或函数 `editor` 参数，绝对路径直接作为覆盖二进制），再依次回退环境变量 `$EDITOR` → `$VISUAL`，最后按默认优先级表（`vscode` → `sublime` → `atom` → `neovim` → `webstorm` → `intellij` → `textmate` → `vim`）结合 macOS `.app` 常见捆绑路径探测。
   - **编辑器行/列跳转参数差异**：VS Code 在提供行号时附加 `--goto file:line[:column]`；Sublime/Atom 使用 `file:line[:column]`；JetBrains（IntelliJ/WebStorm）使用 `file:line`（省略列号）；TextMate 使用 `--line line[:column] file`；终端编辑器（`vim`/`nvim`/`emacs`）自动包装平台打开器（macOS 附加 `--args`）。
   - **跨平台 URL 打开与 `.bun` 临时文件高亮**：macOS 使用 `/usr/bin/open`、Linux 使用 `xdg-open`、Windows 使用 `start`、Android 使用 `am start -a android.intent.action.VIEW -d <url>`、WASI 安全降级为文本打印；`.bun` 结尾的临时文件自动补 `.js` 后缀（`normalizeEditorTempFilename`）以获得编辑器语法高亮。

### 编辑器探测与跳转命令示例（`lib/cli/open.cp`）

```cp
import {
    EditorKind,
    OpenerPlatform,
    resolveEditorFromPreference,
    buildOpenEditorCommand,
} from "cap:cli";

const resolved = resolveEditorFromPreference("", "code --wait", "vim");
let scratch: [256]u8 = [0; 256];
const cmd = buildOpenEditorCommand(
    resolved.kind,
    resolved.binary,
    "/src/main.cp",
    "42",
    "7",
    OpenerPlatform.Linux,
    scratch[0..],
)?;
// cmd.args[0..cmd.argc] => ["code", "--goto", "/src/main.cp:42:7"]
* = cmd;
```

7. **Shell 补全安装路径探测与 Zsh 启动脚本提示（`lib/cli/install_completions.cp`，吸收自 `/data/dev/bun/src/cli/install_completions_command.cp`）**：
   - **三类 Shell 补全文件名**：Fish → `<tool>.fish`，Zsh → `_<tool>`，Bash → `<tool>.completion.bash`；
   - **探测优先级链**：Fish 优先 `$XDG_CONFIG_HOME` → `$XDG_DATA_HOME` → `$HOME/.config/fish/completions` → macOS Homebrew（ARM64 `/opt/homebrew` / x64 `/usr/local`）→ `/etc/fish/completions`；Zsh 优先解析空格分隔的 `$fpath` 再回退 `$XDG_DATA_HOME/zsh-completions` / `$HOME/.oh-my-zsh/completions`；Bash 优先 `$XDG_DATA_HOME` / `$XDG_CONFIG_HOME` / `$HOME/.oh-my-bash/custom/completions` / `$HOME/.bash_completion.d`；
   - **Zsh `$ZDOTDIR` 兼容**：`resolveZshrcPath` 优先定位 `$ZDOTDIR/.zshrc` 再回退 `$HOME/.zshrc`，配合 `zshrcNeedsCompletionHint` 与 `formatZshrcSourceLine` 避免重复追加 `source` 行。

### Shell 补全目录规划示例（`lib/cli/install_completions.cp`）

```cp
import {
    CompletionShell,
    CompletionInstallEnv,
    collectCompletionDirs,
    completionFilename,
} from "cap:cli";

const env = CompletionInstallEnv{
    home: "/Users/dev",
    fpath: "/Users/dev/.zfunc /usr/local/share/zsh/site-functions",
    is_macos: true,
    is_arm64: true,
};
const dirs = collectCompletionDirs(CompletionShell.zsh, env)?;
let fname: [64]u8 = [0; 64];
const fn_len = completionFilename(CompletionShell.zsh, "cap", fname[0..])?;
// dirs.at(0) == "/Users/dev/.zfunc", fname[0..fn_len] == "_cap"
* = dirs;
* = fn_len;
```

8. **包版本递进与 Git 提交消息格式化（`lib/cli/pm_version.cp`，吸收自 `/data/dev/bun/src/cli/pm_version_command.cp` & `/data/dev/bun/docs/pm/cli/pm.mdx`）**：
   - **完整支持 9 类版本策略**：`patch`、`minor`、`major`、`prepatch`、`preminor`、`premajor`、`prerelease`、`from-git` 与具体 Semver（支持剥离可选前导 `v`/`=`）；
   - **预发布标识推导（`--preid`）**：未显式指定 `--preid` 时自动继承当前版本号预发布首段（如 `1.2.3-beta.0` 执行 `prerelease` 递进为 `1.2.3-beta.1`）；对稳定版执行 `prerelease` 时先递进 `patch` 再附 `-0` 或 `-<preid>.0`（参考 `/data/dev/bun/docs/pm/cli/pm.mdx`：`1.0.0` + `prerelease --preid beta` → `1.0.1-beta.0`）；
   - **防误操作与 Git 集成最佳实践**：指定具体版本号且与当前版本一致时默认抛出 `PmVersionError.SameVersion`（需显式开启 `allow_same_version` 放行，对应 `--allow-same-version`）；`formatPmCommitMessage()` 将提交消息模板中所有 `%s` 替换为新版本号（空模板默认输出 `v<version>`）；`normalizeGitTagVersion()` 规范化 `git describe --tags` 输出供 `from-git` 使用。

### 包版本递进与提交消息格式化示例（`lib/cli/pm_version.cp`）

```cp
import {
    PmVersionOptions,
    parsePmVersionArg,
    computeNextPmVersion,
    formatPmCommitMessage,
} from "cap:cli";

const arg = parsePmVersionArg("prerelease")?;
let opts = PmVersionOptions.defaults();
opts.preid = "beta";

let ver_buf: [64]u8 = [0; 64];
const vn = computeNextPmVersion("1.0.0", arg, opts, ver_buf[0..])?;
// ver_buf[0..vn] == "1.0.1-beta.0"

let msg_buf: [128]u8 = [0; 128];
const mn = formatPmCommitMessage("Release beta: %s", ver_buf[0..vn], msg_buf[0..])?;
// msg_buf[0..mn] == "Release beta: 1.0.1-beta.0"
* = mn;
```

9. **`package.json` 点号/方括号路径与赋值解析（`lib/cli/pm_pkg.cp`，吸收自 `/data/dev/bun/src/cli/pm_pkg_command.cp` & `/data/dev/bun/docs/pm/cli/pm.mdx`）**：
   - **四类子命令**：`get` / `set` / `delete` / `fix`（`PmPkgSubCommand`）；
   - **完整路径语法（参考 `/data/dev/bun/docs/pm/cli/pm.mdx`）**：支持点分属性（`scripts.build`）、方括号数组下标（`contributors[0]`）、点分数字下标（`workspaces.0`）、特殊字符方括号键（`scripts[test:watch]`、`exports["./utils"]`）及多维嵌套（`matrix[0][1].name`）；
   - **赋值与 `--json` 类型判定**：`PmPkgAssignment.parse()` 在跳过方括号内 `=` 后拆分 `key=value`；`classifyPmPkgValue(val, true)` 严格区分 `null` / `boolean` / `number` / `string` / `array` / `object` 并拦截畸形 JSON。

### `pm pkg` 路径与赋值解析示例（`lib/cli/pm_pkg.cp`）

```cp
import {
    PmPkgKeyPath,
    PmPkgAssignment,
    classifyPmPkgValue,
} from "cap:cli";

const assign = PmPkgAssignment.parse("scripts[test:watch]=bun test --watch")?;
const kind = classifyPmPkgValue("true", true)?; // PmPkgValueKind.json_bool
let buf: [64]u8 = [0; 64];
const n = assign.path.formatCanonical(buf[0..])?;
// buf[0..n] == "scripts[test:watch]"
* = kind;
* = n;
```

10. **Monorepo `--filter` / `-F` 工作区包过滤与脚本前缀格式化（`lib/cli/filter_arg.cp`，吸收自 `/data/dev/bun/src/cli/filter_arg.cp` & `/data/dev/bun/docs/pm/filter.mdx`）**：
    - **包名与路径双模式匹配**：
      - 包名模式（如 `pkg-*`、`@scope/*`、`*`）匹配 `package.json` 的 `name` 字段；
      - 路径模式（以 `.`、`/` 或 `\` 开头，如 `./packages/*`、`./packages/**`、`./`）自动结合 `cwd` 规范化路径（折叠 `.`/`..`、统一 Windows `\` 为 `/`）后进行 Glob 匹配；
      - 当 `--filter "*"` 与路径规则混用（如 `--filter "*" --filter "./bar"`）时，`match_all` 仅自动匹配非空包名，允许路径规则精准筛选无名包。
    - **`!` 排除规则组合（参考 `/data/dev/bun/docs/pm/filter.mdx`）**：
      - 支持纯排除模式（如 `--filter '!pkg-c'`：默认选中全部候选包再剔除匹配项）及正反混合模式（如 `--filter '!./' --filter './packages/*'`：选中 `./packages/*` 同时排除根 `package.json`）；排除规则优先级高于正向规则。
    - **零递归迭代式 Glob 与遍历跳过守卫**：
      - `matchFilterGlob` 支持 `*`（单段通配）、`**`（跨目录含零层通配）、`?` 与 `[a-z]`/`[!a-z]` 字符类，迭代实现杜绝多星号畸形输入导致的回溯爆炸；
      - `isFilterSkippedDir` 自动过滤 `.`、`node_modules`、`.git`；`formatWorkspacePackagePattern` 将工作区配置（如 `packages/*`）规范化为 `packages/*/package.json`。
    - **Foreman 风格并行/串行脚本输出前缀（`formatFilterScriptPrefix`）**：
      - 配合 `--parallel` / `--sequential` 运行多包脚本时，输出 `<pkg_name>:<script>`（包名为空时自动回退 `<rel_path>:<script>`，均为空时输出 `<script>`）；下游执行器应按工作区依赖拓扑顺序调度（被依赖包先完成）。

### Monorepo `--filter` 过滤集合与脚本前缀示例（`lib/cli/filter_arg.cp`）

```cp
import {
    FilterSet,
    formatWorkspacePackagePattern,
    formatFilterScriptPrefix,
} from "cap:cli";

const raw_filters: [2]([]const u8) = ["!./", "./packages/*"];
const fs = FilterSet.fromPatterns(raw_filters[0..], "/repo")?;
const keep_a = fs.matches("/repo/packages/pkg-a", "pkg-a"); // true
const keep_root = fs.matches("/repo", "monorepo-root");     // false

let pat_buf: [64]u8 = [0; 64];
const pn = formatWorkspacePackagePattern("packages/*", pat_buf[0..])?;
// pat_buf[0..pn] == "packages/*/package.json"

let pfx_buf: [64]u8 = [0; 64];
const fn_len = formatFilterScriptPrefix("pkg-a", "packages/pkg-a", "build", pfx_buf[0..])?;
// pfx_buf[0..fn_len] == "pkg-a:build"
* = keep_a;
* = keep_root;
* = pn;
* = fn_len;
```

11. **`bun test` / `cap test` 测试文件发现与路径过滤引擎（`lib/cli/test_scanner.cp`，吸收自 `/data/dev/bun/src/cli/test/Scanner.cp` & `/data/dev/bun/docs/test/discovery.mdx`）**：
    - **默认发现规则（参考 `/data/dev/bun/docs/test/discovery.mdx`）**：
      - 匹配 `*.test.{js,jsx,ts,tsx,mjs,cjs,mts,cts,cp}`、`*_test.*`、`*.spec.*` 与 `*_spec.*`（`couldBeTestFile` / `hasTestNameSuffix`）；
      - 默认跳过隐藏目录（`.` 开头）与 `node_modules`（`TestFileScanner.shouldScanDir`）。
    - **位置过滤词 vs 精确文件路径**：
      - 无前导 `./`/`../`/`/` 的位置参数按路径子串过滤（如 `bun test utils` 命中 `src/utils/string.test.ts`）；
      - 指定精确文件需以 `./` 或 `/` 开头（`isExactTestPathArg`）；
      - 按测试名过滤（`-t` / `--test-name-pattern`）将全部外层 `describe` 标签与测试名以空格拼接（`formatScopedTestName`）。
    - **`path_ignore_patterns` 智能目录剪枝**：
      - 仅对包含 `**` 的忽略模式（如 `vendor/**`）执行子树提前剪枝，单 `*` 模式（如 `vendor/*`）不剪枝子目录以避免误跳过深层文件。

### 测试文件发现与忽略剪枝示例（`TestFileScanner`）

```cp
import { TestFileScanner, formatScopedTestName } from "cap:cli";

let sc = TestFileScanner.init();
sc.addFilter("utils")?;
sc.addPathIgnorePattern("vendor/**")?;

const keep = sc.shouldIncludeFile("/repo/src/utils/str.test.ts", "src/utils/str.test.ts", true); // true
const prune = sc.shouldScanDir("vendor", "vendor"); // false

const labels: [2]([]const u8) = ["Math", "operations"];
let name_buf: [64]u8 = [0; 64];
const n = formatScopedTestName(labels[0..], "should add", name_buf[0..])?;
// name_buf[0..n] == "Math operations should add"
* = keep;
* = prune;
* = n;
```

12. **包安全漏洞审计引擎与报告格式化（`lib/cli/pm_audit.cp`，吸收自 `/data/dev/bun/src/cli/audit_command.cp` & `/data/dev/bun/docs/pm/cli/audit.mdx`）**：
    - **严重性分级与阈值过滤（`--audit-level=<low|moderate|high|critical>`）**：
      - `PmAuditSeverity.parse` 严格大小写无关解析 4 个等级，`parseOrDefault` 遵循上游宽容策略将未知级别归为 `Moderate`；`meetsThreshold` 按 `Low < Moderate < High < Critical` 过滤低危条目。
    - **CVE/GHSA 忽略与生产依赖范围（`--ignore` / `--prod`）**：
      - `PmAuditFilter.shouldInclude` 同时支持精确匹配公告 `id` 与公告 `url` 子串命中（可叠加多个 `--ignore CVE-...`）；
      - `PmAuditRequestBuilder` 自动跳过非默认 Registry 包并累计 `skipped_non_default_count`，在 `prod_only = true` 时排除 `devDependencies`，并通过 `formatBulkAdvisoryJson` 零堆分配序列化 RFC 8259 转义后的 NPM Bulk Advisory 请求 JSON。
    - **退出码契约与无环依赖链追踪**：
      - 对齐 `/data/dev/bun/docs/pm/cli/audit.mdx` 规范：无论是否启用 `--json`，无漏洞返回退出码 `0`（摘要 `"No vulnerabilities found"`），存在任意漏洞返回退出码 `1` 并附带 `cap update` / `cap update --latest` 升级建议；
      - `PmAuditDepGraph.findDependentChain` 以访问集守卫反向追踪引入漏洞包的依赖链，安全免疫循环依赖。

### 包安全审计与 Bulk Advisory 请求构建示例（`lib/cli/pm_audit.cp`）

```cp
import {
    PmAuditSeverity,
    PmAuditFilter,
    PmAuditRequestBuilder,
    PmAuditAdvisory,
    PmAuditReport,
} from "cap:cli";

// 1. 构建 NPM Bulk Advisory 请求体（仅生产依赖、自动跳过私有源）
let req = PmAuditRequestBuilder.init(true);
* = req.addPackage("lodash", "4.17.20", true, true)?;
* = req.addPackage("vitest", "1.0.0", false, true)?;
let json_buf: [256]u8 = [0; 256];
const jn = req.formatBulkAdvisoryJson(json_buf[0..])?;
// json_buf[0..jn] == "{\"lodash\":[\"4.17.20\"]}"

// 2. 过滤与聚合审计报告
let filter = PmAuditFilter.init();
filter.min_level = PmAuditSeverity.High;
* = filter.addIgnore("CVE-2022-25883")?;

let report = PmAuditReport.init();
* = report.addAdvisory(PmAuditAdvisory{
    package_name: "lodash",
    id: "GHSA-jf85-cpcp-j695",
    severity: PmAuditSeverity.High,
    title: "Prototype Pollution",
    url: "https://github.com/advisories/GHSA-jf85-cpcp-j695",
    vulnerable_versions: "<4.17.21",
}, filter)?;

let sum_buf: [128]u8 = [0; 128];
const sn = report.formatSummaryLine(sum_buf[0..])?;
// sum_buf[0..sn] == "1 vulnerability (1 high)", report.exitCode() == 1
* = jn;
* = sn;
```

13. **依赖过期分析、Catalog 分组与对齐表格引擎（`lib/cli/pm_outdated.cp`，吸收自 `/data/dev/bun/src/cli/outdated_command.cp` & `/data/dev/bun/docs/pm/cli/outdated.mdx`）**：
    - **三列版本语义（Current / Update / Latest）**：
      - `Current` 为当前已安装版本，`Update` 为满足 `package.json` 范围约束的最新兼容版本，`Latest` 为 Registry 发布的最新版本；
      - `PmOutdatedSemver.classifyUpgradeTo` 精确区分 `UpToDate` / `Patch` / `Minor` / `Major` / `Prerelease`，`PmOutdatedTable.addEntry` 自动剔除已为最新的依赖项。
    - **包名通配与 `!` 排除过滤**：
      - 支持位置参数正向匹配（如 `eslint*`、`@types/*`）与反向排除（如 `!@types/*`），迭代式 `matchOutdatedGlob` 零递归免疫 ReDoS。
    - **Monorepo Workspace 与 Catalog 依赖分组**：
      - 同一个默认或命名 Catalog（如 `catalog:testing`）在多个 Workspace 中被引用时，`PmOutdatedTable.addEntry` 自动按 `(package_name, catalog_name, dep_kind)` 合并去重并格式化为 `catalog:testing (@app/a, @app/b)`；
      - `sortDeterministic()` 按依赖类别（`Normal < Dev < Peer < Optional`）与包名字母序稳定排列，`formatMarkdownTable()` 动态计算列宽输出对齐表格。

### 依赖过期检查与 Catalog 表格渲染示例（`lib/cli/pm_outdated.cp`）

```cp
import {
    PmOutdatedDepKind,
    PmOutdatedFilter,
    PmOutdatedEntry,
    PmOutdatedTable,
} from "cap:cli";

let filter = PmOutdatedFilter.init();
* = filter.addPattern("!@types/*")?;

let tbl = PmOutdatedTable.init();
* = tbl.addEntry(PmOutdatedEntry{
    package_name: "eslint",
    dep_kind: PmOutdatedDepKind.Dev,
    current: "8.57.1",
    update: "8.57.1",
    latest: "9.20.0",
    workspace_name: "@app/web",
    is_catalog: true,
    catalog_name: "testing",
}, filter)?;
tbl.sortDeterministic();

let out: [512]u8 = [0; 512];
const n = tbl.formatMarkdownTable(out[0..], true)?;
// 包含 "| eslint (dev) | 8.57.1  | 8.57.1 | 9.20.0 | catalog:testing (@app/web) |"
* = n;
```

14. **生命周期脚本信任策略与 `trustedDependencies` 供应链防护引擎（`lib/cli/pm_trusted.cp`，吸收自 `/data/dev/bun/src/cli/pm_trusted_command.cp` & `/data/dev/bun/docs/pm/lifecycle.mdx`）**：
    - **默认安全（Default-Secure）原则**：不默认执行第三方依赖的 `preinstall` / `install` / `postinstall` / `prepare` 等任意脚本，仅当包命中显式 `trustedDependencies` 或（仅限 npm Registry 源）默认信任白名单时才允许运行；
    - **非 Registry 源防伪造红线**：对 `file:`、`link:`、`git:`、`github:`、`tarball` 来源的包，即便包名与默认白名单（如 `esbuild`）重名，也强制返回 `PmTrustDecision.BlockedNonRegistrySpoof`，必须由用户显式加入 `trustedDependencies` 才可放行；
    - **全局 `--ignore-scripts` 覆盖与白名单持久化**：`ignore_scripts = true` 拥有最高阻断优先级；支持作用域包与版本锁定条目（`"@swc/core@1.7.0"`）、字典序排序（`sortExplicitAllowlist`）、JSON 数组序列化（`formatTrustedJsonArray`）与 `pm untrusted` 审阅报告格式化（`formatUntrustedReport`）。

### 生命周期脚本信任评估与报告示例（`lib/cli/pm_trusted.cp`）

```cp
import {
    PmDepSourceKind,
    PmLifecyclePhase,
    PmLifecyclePackage,
    PmTrustPolicy,
} from "cap:cli";

let pol = PmTrustPolicy.initWithBuiltins(false);
* = pol.addExplicitTrust("node-sass")?;

let pkg_git = PmLifecyclePackage{ name: "esbuild", version: "0.24.0", source: PmDepSourceKind.Git };
pkg_git.addScript(PmLifecyclePhase.Postinstall);
// 非 npm 源伪造同名默认信任包 -> PmTrustDecision.BlockedNonRegistrySpoof
const decision = pol.evaluate(pkg_git);
* = decision;
```

15. **Monorepo 工作区脚本 DAG 调度器与输出折叠器（`lib/cli/filter_run.cp`，吸收自 `/data/dev/bun/src/cli/filter_run.cp` & `/data/dev/bun/docs/pm/filter.mdx`）**：
    - **依赖拓扑顺序与环回退（`FilterRunGraph`）**：
      - 忠实还原 Bun `bun --filter <pattern> <script>` / `--workspaces` 执行调度模型：若工作区包 `app` 依赖 `core`，则 `core` 的脚本全部完成后才启动 `app`；
      - 内置**迭代式三色 DFS 环检测**（零递归，免疫深链栈溢出）：当跨包依赖存在循环时，`resolveDependencies()` 标记 `cycle_detected = true` 并清除跨包依赖边回退为并行启动，同时**始终保留**同包内 `pre<script>` -> `<script>` -> `post<script>` 串行生命周期顺序；
      - 支持三种调度模式：`FilterRunMode.Topological`（默认拓扑并发）、`FilterRunMode.Parallel`（对应 `--parallel`，忽略跨包边全并发）与 `FilterRunMode.Sequential`（对应 `--sequential`，按序逐一串行）；
      - 支持 `exit_on_error` 开关：默认为 `true`（任一脚本非零退出/信号终止即中止所有 `Pending` 任务），设为 `false` 时对齐 `--no-exit-on-error` 继续完成其余独立包脚本；`finalizeExitCode()` 按 Bun 语义聚合最终退出码（非零退出码优先，信号终止映射为 `128 + signal`，错误映射为 `1`）。
    - **输出尾部行折叠与框线渲染（`elideScriptOutput` / `formatFilterOutputBlock`）**：
      - `elideScriptOutput(data, max_lines)` 零分配剥离末尾单个 `\n` 并保留最后 `max_lines` 行（默认 10 行），返回被折叠行数 `elided_count`；`is_abort = true` 时展开完整日志便于排障；
      - `formatFilterDuration`（`> 1000 ms` 渲染 `Done in X.YY s`，否则 `Done in N ms`）、`formatFilterStatusLine` 与 `formatFilterOutputBlock` 生成标准 `│` / `└─` 框线输出块。

### Monorepo 拓扑调度与输出折叠示例（`lib/cli/filter_run.cp`）

```cp
import {
    FilterRunMode,
    FilterScriptPhase,
    FilterRunGraph,
    elideScriptOutput,
    formatFilterOutputBlock,
} from "cap:cli";

let g = FilterRunGraph.init(FilterRunMode.Topological, true);
const core_pre = g.addScript("@ws/core", "prebuild", "rm -rf dist", FilterScriptPhase.Pre)?;
const core_main = g.addScript("@ws/core", "build", "tsc -b", FilterScriptPhase.Main)?;
const app_main = g.addScript("@ws/app", "build", "vite build", FilterScriptPhase.Main)?;

// app 依赖 core：core 的 prebuild -> build 全部完成后才会就绪 app:build
* = g.addPackageDependency("@ws/core", "@ws/app")?;
const has_cycle = g.resolveDependencies()?; // false

let ready: [8]usize = [0; 8];
const n1 = g.collectReady(ready[0..]); // 仅 core_pre 就绪 (n1 == 1)
g.markStarted(core_pre)?;
g.markExited(core_pre, 0, 120_000_000)?; // 120 ms

g.markStarted(core_main)?;
g.markExited(core_main, 0, 1_450_000_000)?; // 1.45 s -> 释放 app_main

const n2 = g.collectReady(ready[0..]); // app_main 就绪 (n2 == 1)
g.markStarted(app_main)?;
g.markExited(app_main, 0, 800_000_000)?;

const elided = elideScriptOutput("l1\nl2\nl3\nl4\n", 2);
// elided.content == "l3\nl4", elided.elided_count == 2
let box_buf: [512]u8 = [0; 512];
const bn = formatFilterOutputBlock(g.nodes[core_main], "ok\n", false, box_buf[0..])?;
* = has_cycle;
* = n1;
* = n2;
* = elided;
* = bn;
```

16. **`bunx` / `bun x` 包可执行文件解析、Shebang 运行时路由与 24h 缓存管理（`lib/cli/bunx.cp`，吸收自 `/data/dev/bun/src/cli/bunx_command.cp` & `/data/dev/bun/docs/pm/bunx.mdx`）**：
    - **严格位置语义的参数解析（`BunxOptions.parse`）**：
      - `--bun` / `-b`、`--package <pkg>` / `-p <pkg>`、`--no-install`、`--verbose`、`--silent` 等 `bunx` 标志必须出现在目标包/可执行文件**之前**；出现在目标名之后的所有参数（包括 `--bun`）原样透传给下游可执行文件，零拷贝切片通过 `passthroughSlice(argv)` 获取；
      - 当包名与可执行文件名不一致时（如 `bunx -p @angular/cli ng` 或 `bunx --package=renovate renovate-config-validator`），自动校验必须同时提供目标可执行文件名。
    - **模板包前缀与作用域说明符解析**：
      - `formatBunxCreatePackageName` 完整对齐 `bun create` / `bunxAddCreatePrefix` 规则（`vite` -> `create-vite`、`@scope/app` -> `@scope/create-app`、`@scope@1.0` -> `@scope/create@1.0`、`@scope` -> `@scope/create`）；
      - `parseBunxPackageSpec` 与 `resolveBunxDefaultBinName` 支持作用域包与 `@version` 标签拆解，并从 `package.json` 的 `"bin"` 路径提取基础文件名（`extractBinBasename`）。
    - **Shebang 运行时探测与 24 小时临时缓存防护**：
      - `resolveBunxShebangRuntime` 尊重脚本首行 `#!/usr/bin/env node` / `#!/usr/bin/env bun`（支持 `env -S` 标志），并在 `--bun`（`force_bun = true`）时强制切换至 Bun 运行时；
      - `isBunxCacheStale` 按 `BUNX_CACHE_TTL_SECS = 86400`（24 小时）判定缓存过期；`formatBunxCacheDirName` 拒绝 `..` 路径穿越与控制字符，安全规范化作用域分隔符。

### `bunx` 参数解析与 Shebang 运行时路由示例（`lib/cli/bunx.cp`）

```cp
import {
    BunxOptions,
    BunxRuntimeChoice,
    formatBunxCreatePackageName,
    parseBunxPackageSpec,
    resolveBunxShebangRuntime,
} from "cap:cli";

const argv: [6]([]const u8) = ["bunx", "--bun", "-p", "@angular/cli@17", "ng", "version"];
const opts = BunxOptions.parse(argv[0..], true)?;
// opts.run_in_bun == true, opts.package_name == "@angular/cli@17", opts.binary_name == "ng"
const pass = opts.passthroughSlice(argv[0..]); // ["version"]

const spec = parseBunxPackageSpec(opts.package_name)?; // package_name="@angular/cli", version="17"
const rt = resolveBunxShebangRuntime("#!/usr/bin/env node\nconsole.log(1);", opts.run_in_bun);
// rt == BunxRuntimeChoice.Bun (--bun 覆盖 node shebang)

let create_buf: [64]u8 = [0; 64];
const cn = formatBunxCreatePackageName("@acme/app@2", create_buf[0..])?;
// create_buf[0..cn] == "@acme/create-app@2"
* = pass;
* = spec;
* = rt;
* = cn;
```

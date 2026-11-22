# `cap:shell` — 嵌入式跨平台 Shell 词法/AST 引擎与进程内/外混合管道执行器

> 参考来源：`/data/dev/bun/docs/runtime/shell.mdx`, `/data/dev/bun/src/shell/`, `/data/dev/bun/src/shell_parser/braces.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/shell/`  
> **导入协议**: `import { Shell, sh, exec, ShellSession, ShellEnv, DirStack, ShellOptions, AliasMap, BraceExpander, ArithEval, expandWord, expandWordWithSession, Lexer, Token, TokenKind, QuoteKind, Parser, Script, Statement, Pipeline, Command, CommandKind, ControlOp, Executor, ShellResult, isBuiltin, runBuiltin, runBuiltinWithState } from "cap:shell";`

---

## 一、 架构概述与模块拓扑

`cap:shell` 基于 Bun 内置跨平台 Shell 引擎（`Bun.$` / `bun/src/shell`）直迁构建，将全功能 POSIX 词法分析、递归下降复合语句 AST、大括号/算术/命令替换/Glob 展开、子 Shell 状态隔离、`PosixSpawn` 外部进程与内置命令混合管道以及 30+ 生产级 Builtins 融为一体：

```text
lib/shell/
├── index.cp         # 模块导出门面与快捷同步执行入口 sh(...) / exec(...) / Shell.run(...) / Shell.session()
├── lexer.cp         # POSIX 词法分词器 (单/双/ANSI-C $'...' 引号、转义、${...}/$(...)/$((...))/`...`、# 注释、全套重定向符)
├── braces.cp        # 大括号展开引擎 ({a,b,c}、嵌套 {a,{b,c}}、笛卡尔积、数字/字母/步长区间 {1..5}/{01..05}/{1..10..2}/{a..e})
├── env.cp           # 进程内环境变量表 (ShellEnv)、目录栈 (DirStack)、ShellOptions、AliasMap 与词展开引擎
├── arith.cp         # POSIX 整数算术求值器 $(( ... )) (四则/幂/位运算/比较/逻辑/三目/赋值/自增自减与环境读写)
├── parser.cp        # 复合 AST 解析器 (AstPool, Script, Pipeline, Command, if/elif/else/fi, for, while/until, (...), {...;})
├── builtinFs.cp    # 文件系统 Builtins (ls, cat, mkdir, rmdir, rm, cp, mv, touch, basename, dirname, mktemp)
├── builtinText.cp  # 文本与流处理 Builtins (seq, head, tail, wc, grep, yes, sort, uniq)
├── builtin.cp       # 控制/环境 Builtins (echo, printf, pwd, cd, pushd, popd, dirs, env, export, unset, which, true, :, false, test/[, exit) 与统一调度器
└── exec.cp          # 会话执行与混合管道引擎 (ShellSession, Executor, GlobWalker 集成, Subshell 隔离, PosixSpawn 外部进程管道)
```

1. **完整 POSIX 词法 & 展开链**：
   - 展开管线按 POSIX 标准顺序执行：**大括号展开 `{...}` → 波浪号 `~` → 参数/算术/命令替换 (`$VAR`/`${VAR:-def}`/`${VAR:=val}`/`${VAR:+alt}`/`${#VAR}`/`$?`/`$$`/`$#`/`$0..$9`/`$@`/`$((...))`/`$(...)`/`` `...` ``) → 无引号分词 → Glob 路径展开 (`*`, `?`, `[...]`) → 去引号**。
   - 单引号 `'...'` 屏蔽所有展开；双引号 `"..."` 支持插值与 `\"`/`\\`/`\$`/`\`` 转义；`$ '...'`（ANSI-C `$'...'`）支持 `\n`/`\t`/`\r`/`\\`/`\'` 转义。
2. **完整控制流与子 Shell 隔离**：
   - 支持 `if ... then ... elif ... else ... fi`、`for x in ...; do ...; done`、`while ...; do ...; done`、`until ...; do ...; done`、`break`/`continue`、`&&`/`||`/`;`/`\n` 以及管道取反 `!`。
   - 支持 `( ... )` 子 Shell 环境隔离（`ShellSession.cloneForSubshell()` 隔离环境变量、目录栈、`cd` 工作目录、`export`、`alias` 与 `exit`）与 `{ ...; }` 当前会话命令组。
   - 支持 `set`（`-e`/`-u`/`-x`/`-o pipefail`/`--`）、`shift`、`alias`/`unalias`。
3. **多级混合真实管道 & 全套重定向**：
   - 内置命令（零 fork）与外部可执行文件（`cap:sys` `PosixSpawn` + `cap:io` `Pipe` + `poll` 非阻塞读写）可在同一条多级管道中自由混排（如 `printf ... | tr a-z A-Z | sort | head -n 2`），并自动继承 `ShellEnv` 导出变量。
   - 支持 `<`、`<<<`（Here-String）、`>`、`>>`、`2>`、`2>>`、`2>&1`、`1>&2`、`&>`、`&>>` 以及 `ShellSession.runWithMemoryTarget` 内存缓冲区直出。

---

## 二、 语法特性与 Builtins 对照表

| 类别 | 特性 / 命令 | 语义与支持能力 | 示例 |
| :--- | :--- | :--- | :--- |
| **参数与算术展开** | `${VAR:-/=/+/ #}`, `$((...))`, `$(...)`, `` `...` `` | 默认值/赋值/替代/长度、全套整数算术与环境变量写回、隔离命令替换 | `echo $((a += 2)) $(pwd)` |
| **大括号与路径展开** | `{a,b}`, `{1..5}`, `*`, `?`, `[...]` | 嵌套大括号与区间展开、基于 `cap:glob` 的单层/递归 Glob 匹配 | `echo src/{a,b}_*.cp` |
| **控制流与隔离** | `if/elif/else/fi`, `for`, `while/until`, `(...)`, `{...;}` | 完整分支与循环、`break`/`continue`、子 Shell 环境变量与 `cwd` 隔离 | `(cd /tmp && pwd); pwd` |
| **管道与重定向** | `\|`, `!`, `&&`, `\|\|`, `>`, `>>`, `<`, `2>`, `2>&1`, `&>`, `<<<` | Builtin 与 `PosixSpawn` 外部进程混排管道、`pipefail`、Here-String | `grep -i hi <<< 'Hi!' \| tr a-z A-Z` |
| **环境与会话** | `pwd`, `cd`, `pushd`, `popd`, `dirs`, `env`, `export`, `unset`, `which`, `set`, `shift`, `alias`, `unalias`, `true`, `:`, `false`, `exit` | 目录栈、OS `environ` 合并与导出隔离、`cap:which` 可执行查找 | `export K=v && env \| grep K` |
| **条件测试** | `test` / `[` | `-e/-f/-d/-s/-L/-h/-r/-w/-x`、`-z/-n/=/!=/==`、`-eq/-ne/-lt/-le/-gt/-ge`、`!/-a/-o` | `[ -f a.txt -a 10 -gt 2 ]` |
| **文件与目录** | `ls`, `cat`, `mkdir`, `rmdir`, `rm`, `cp`, `mv`, `touch`, `basename`, `dirname`, `mktemp` | `ls -la1dr`, `cat -n`, `mkdir -p`, `rm -rf`, `cp -r`, `mv -n`, `mktemp -d` | `mkdir -p d && touch d/a && rm -rf d` |
| **文本与序列** | `echo`, `printf`, `seq`, `head`, `tail`, `wc`, `grep`, `yes`, `sort`, `uniq` | `seq -s -w`, `head/tail -n/-c`, `wc -lwc`, `grep -ivcnlqFE`, `yes -n`, `sort -rnu`, `uniq -c` | `seq 1 5 \| sort -r \| head -n 2` |

---

## 三、 代码示例

### 1. 有状态会话 (`ShellSession`)、展开、控制流与子 Shell 隔离

```typescript
import { Shell } from "cap:shell";

fn demoControlAndSubshell(): void {
    let sess = Shell.session();
    defer sess.release();

    const r = sess.run(
        "MODE=prod; " +
        "(MODE=dev; cd /tmp && echo sub=$MODE:$(pwd)); " +
        "if [ \"$MODE\" = \"prod\" ]; then " +
        "  for i in {1..3}; do echo -n $((i * 10)),; done; " +
        "fi"
    );
    // r.ok() == true
    // r.text() => "sub=dev:/tmp\n10,20,30,"
}
```

### 2. Builtin + `PosixSpawn` 外部进程混合管道与内存重定向

```typescript
import { Shell } from "cap:shell";
import { StringJoiner } from "cap:string";

fn demoMixedPipeline(): void {
    let sess = Shell.session();
    defer sess.release();

    let out = StringJoiner.init();
    defer out.release();
    let err = StringJoiner.init();
    defer err.release();

    const code = sess.runWithMemoryTarget(
        "printf 'banana\\napple\\n' | tr a-z A-Z | sort | head -n 1",
        &out,
        &err,
    );
    // code == 0, out.slice() => "APPLE\n"
}
```

---

## 附加零分配子模块

- **`lib/shell/envparse.cp`**（POSIX Shell 环境变量赋值解析）：`ShellEnvEntry`、`ShellEnvIter`、`getShellEnv`
- **`lib/shell/quote.cp`**（POSIX Shell 引用转义与参数切分）：`needsQuoting`、`quotePosix`、`joinArgs`、`ShellArgIter`
- **`lib/shell/brace.cp`**（Bash 风格花括号展开零分配迭代器）：`BraceExpandIter`
- **`lib/shell/condExpr.cp`**（吸收自 `/data/dev/bun/src/shell/states/CondExpr.cp`，零堆分配 Bash `[[ ... ]]` 条件表达式解析与求值器）：`MAX_COND_DEPTH`、`POSIX_S_*`、`CondExprError`、`CondOp`、`CondFileStat`、`CondStatFn`、`condToExitCode`、`CondExprEvaluator`（支持 `-e/-f/-d/-c/-L/-s/-r/-w/-x` 文件测试、`-z/-n/==/!=/</>` 字符串测试、`-eq/-ne/-lt/-le/-gt/-ge` 64 位整数比较、`&&`/`||`/`!`/`(...)` 逻辑组合与递归深度防护）
- **`lib/shell/echo.cp`**（吸收自 `/data/dev/bun/src/shell/builtin/echo.cp`，零堆分配 GNU/Bash `echo` 标志解析与反斜杠转义展开引擎）：`EchoError`、`EchoOptions`、`EchoEscapeResult`、`expandEchoEscapes`、`formatEchoArgs`（支持 `-n`/`-e`/`-E` 组合标志、`\\`/`\a`/`\b`/`\c`/`\e`/`\f`/`\n`/`\r`/`\t`/`\v`/`\0NNN`/`\xHH` 转义与 `\c` 提前截断）
- **`lib/shell/trampoline.cp`**（吸收自 `/data/dev/bun/src/shell/Yield.cp`，零堆分配栈安全 Shell Trampoline & Continuation 调度内核）：`MAX_TRAMPOLINE_REENTRY`、`MAX_PIPELINE_STACK_DEPTH`、`DEFAULT_MAX_TRAMPOLINE_STEPS`、`TrampolineError`、`YieldKind`、`PipelinePhase`、`ShellYield`、`PipelineStack`、`TrampolineResult`、`ShellTrampoline`（支持非递归状态机步进、嵌套管道栈自顶向下 `drainPipelines` 排空、`io_writer_chunk` 续跑、RAII `~init()` 重入深度守卫与步数上限防护）

### 栈安全 Trampoline & Continuation 调度示例（`trampoline.cp`）

```typescript
import { ShellTrampoline, ShellYield, PipelinePhase } from "cap:shell";

let tramp = ShellTrampoline.init();
const res = tramp.run(MyCtx, &ctx, ShellYield.forNode(.pipeline, 1), myStepFn, myPhaseFn)?;
// res.final_yield, res.steps_executed, res.pipelines_drained, res.max_pipeline_depth
```

- **`lib/shell/mkdirOpts.cp`**（吸收自 `/data/dev/bun/src/shell/builtin/mkdir.cp`，零堆分配 POSIX `mkdir` 选项解析与父目录规划器）：`DEFAULT_MKDIR_MODE` (`0o777`)、`MkdirParseError`、`parseOctalMode`、`MkdirOpts`（支持 `-p`/`--parents`、`-v`/`--verbose`、`-m`/`--mode` 组合短标志与 `--` 终结符）、`ParentDirIter`（零分配逐级父目录前缀迭代器）

```typescript
import { MkdirOpts, ParentDirIter } from "cap:shell";

const args = ["-pv", "-m", "0755", "/var/log/app"];
const opts = MkdirOpts.parse(args[0..])?; // parents=true, verbose=true, effectiveMode()==0o755
let it = ParentDirIter.init(args[opts.paths_start]); // "/var" -> "/var/log" -> "/var/log/app"
```

### GNU/Bash `echo` 格式化与转义示例（`echo.cp`）

```typescript
import { formatEchoArgs } from "cap:shell";

const args = ["-ne", "hello\\tworld\\x21"];
let buf: [64]u8 = undefined;
const out = formatEchoArgs(args[0..], buf[0..])?; // "hello\tworld!"
```

### Bash `[[ ... ]]` 条件表达式求值示例（`condExpr.cp`）

```typescript
import { CondExprEvaluator, CondFileStat, condToExitCode } from "cap:shell";

const tokens = ["(", "hello", "==", "hello", ")", "&&", "42", "-ge", "10"];
const ok = CondExprEvaluator.evalTokens(tokens[0..], null)?;
const exit_code = condToExitCode(ok); // 0
```

### POSIX/GNU `seq` 数字序列解析与格式化示例（`seq.cp`，吸收自 `/data/dev/bun/src/shell/builtin/seq.cp`）

- 导出：`DEFAULT_SEQ_MAX_STEPS`、`SeqError`、`SeqOptions`（支持 `-s`/`--separator`、`-t`/`--terminator`、`-w`/`--fixed-width`、`--` 与 1/2/3 位置参数及步进方向校验）、`SeqIter`（零堆分配惰性迭代器，使用 `start + step * increment` 消除浮点累加漂移）、`formatSeq`、`formatSeqArgs`（内置 `max_steps` 防资源耗尽保护）。

```typescript
import { formatSeqArgs } from "cap:shell";

pub fn demoSeq(allocator: std.mem.Allocator): ! {
    const args: [4][]const u8 = ["-w", "-s", ",", "8", "10"];
    const out = formatSeqArgs(allocator, args[0..])?; // "08,09,10,"
    defer allocator.free(out);
}
```

### POSIX `exit` 退出码解析与 `export` 环境变量声明编解码（`exitExport.cp`，吸收自 `/data/dev/bun/src/shell/builtin/{exit,export}.cp`）

- 导出：`SHELL_EXIT_SUCCESS` (`0`)、`SHELL_EXIT_GENERAL_ERROR` (`1`)、`SHELL_EXIT_MISUSE` (`2`)、`SHELL_EXIT_NOT_EXECUTABLE` (`126`)、`SHELL_EXIT_NOT_FOUND` (`127`)、`SHELL_EXIT_SIGNAL_BASE` (`128`)、`ShellExitExportError`、`ExportEntry`、`isValidShellIdentifier`、`parseExitCodeStr`（逐位模 256 解析，支持任意大十进制与负整数永不溢出）、`parseExitArgs`、`exitCodeFromSignal`、`isSignalExitCode`、`parseExportArg`、`formatExportLine`。

```typescript
import { parseExitCodeStr, parseExportArg, formatExportLine } from "cap:shell";

const code = parseExitCodeStr("257")?; // 1 (257 mod 256)
const entry = parseExportArg("APP_ENV=production")?; // name="APP_ENV", value="production", has_value=true
let buf: [64]u8 = [0; 64];
const line = formatExportLine(buf[0..], entry.name, entry.value)?; // "APP_ENV=production\n"
* = code;
* = line;
```

### POSIX `cd` 目标解析、路径规范化与 `(cwd, prev_cwd)` 状态机（`cdPath.cp`，吸收自 `/data/dev/bun/src/shell/builtin/cd.cp`）

- 导出：`SHELL_CD_MAX_PATH` (`1024`)、`SHELL_CD_MAX_SEGMENTS` (`64`)、`ShellCdError`、`CdTargetKind`（`Home` / `Previous` / `Path`）、`CdTarget`（`parseArgs`）、`normalizePosixPath`、`resolveCdTarget`、`formatCdError`、`ShellDirState`（`init` / `cwd` / `prevCwd` / `applyCd`）。

```typescript
import { ShellDirState } from "cap:shell";

let dir = ShellDirState.init("/workspace/app")?;
const args1: [1][]const u8 = ["../shared/./lib"];
* = dir.applyCd(args1[0..], "/home/dev")?; // "/workspace/shared/lib"
const args2: [1][]const u8 = ["-"];
* = dir.applyCd(args2[0..], "/home/dev")?; // "/workspace/app"
```

### POSIX `mv` 标志解析与多源目标路径规划器（`mvOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/mv.cp`）

- 导出：`SHELL_MV_MAX_SOURCES` (`64`)、`ShellMvError`、`MvOverwritePolicy`（`Force` / `Interactive` / `NoClobber`）、`MvOpts`（`forceOverwrite` / `interactiveMode` / `noOverwrite` / `applyShortFlag`）、`MvPlan`（`parse` / `requiresDirectoryTarget` / `resolveDestPath` / `formatVerboseLine`）。
- 遵循 POSIX/Bun 互斥覆盖策略规则：`-f`、`-i`、`-n` 后出现者覆盖先出现者；支持合并短标志（如 `-fvn`）与 `--` 选项终止哨兵（允许移动 `-` 开头的畸形文件名）；当源数量大于 1 且目标非目录时确定性返回 `TargetMustBeDirectory`。

```cp
import { MvPlan } from "cap:shell";

const args: [4][]const u8 = ["-vn", "src/a.cp", "src/b.cp", "dist/"];
const plan = MvPlan.parse(args[0..])?;
let dst: [128]u8 = [0; 128];
const n = plan.resolveDestPath(0, true, dst[0..])?; // "dist/a.cp"
* = n;
```

---

## 收敛扩展子模块（`posixlexer.cp` / `posixast.cp` / `posixquote.cp` / `util.cp`）

- **独立 POSIX Shell 词法/AST 解析与转义 (`posixlexer.cp` / `posixast.cp` / `posixquote.cp`)**：`ShellError`, `PosixTokenKind`, `ShellToken`, `deinitTokens`, `tokenize`, `RedirectKind`, `PosixRedirect`, `Assignment`, `SimpleCommand`, `PosixPipeline`, `ListOp`, `ListItem`, `ShellScript`, `parseScript`, `posixNeedsQuoting`, `quoteArg`, `joinCommand`（支持 `cap.shell.*` 动态挂载）。
- **零堆分配 POSIX 转义与路径工具 (`util.cp`)**：`EnvAssignment`, `isSafeUnquoted`, `quotePosixBuf`, `escapeDoubleQuoted`, `joinCommandBuf`, `posixBasename`, `posixDirname`, `isValidEnvVarName`, `splitEnvAssignment`。

---

## 上游 Bun Shell 行为契约与安全防御注意事项

1. **命令注入防线（默认安全引用转义）**：
   - 参考 `/data/dev/bun/docs/runtime/shell.mdx`：向动态构造的 Shell 命令拼接外部用户输入时，务必使用 `quotePosix` / `quotePosixBuf` / `joinArgs` 对所有非安全字面量参数执行 POSIX 单引号安全转义（仅 `isSafeUnquoted` 返回 `true` 的安全字符集可免引号），彻底阻断元字符注入。
2. **原生内置命令跨平台一致性**：
   - `ls`、`cd`、`rm`、`cp`、`mv`、`mkdir`、`cat`、`grep`、`head`、`tail`、`wc`、`sort`、`uniq` 等 30+ 常用命令均在进程内原生实现（`isBuiltin` / `runBuiltinWithState`），无需依赖外部 `/bin/sh` 或第三方工具，并可在同一 `ShellSession` 内维护独立工作目录与 `ShellEnv` 而不污染宿主进程全局环境。
3. **子 Shell 状态隔离（`(...)` vs `{ ...; }`）**：
   - 括号子 Shell `(...)` 会克隆当前 `ShellSession` 的环境变量与目录栈，内部 `cd`/`export` 不会泄漏至父会话；花括号命令组 `{ ...; }` 则在当前会话中直接执行。
4. **POSIX 展开顺序契约**：
   - 严格遵循 `大括号展开 -> 波浪号 -> 参数/算术/命令替换 -> 无引号分词 -> Glob 展开 -> 去引号` 顺序；单引号 `'...'` 禁止一切展开，双引号 `"..."` 允许变量替换但禁止 Glob 与分词。
5. **重定向、管道、工作目录与流式按行读取（吸收自 `/data/dev/bun/docs/runtime/shell.mdx` 与 `/data/dev/bun/docs/guides/runtime/shell.mdx`）**：
   - 完整支持标准文件描述符重定向（`<`、`>`/`1>`、`2>`、`&>`、`>>`/`2>>`/`&>>`、`1>&2`、`2>&1`）及内存缓冲/流对象互连，无需落地临时文件；
   - 工作目录切换支持单命令局部 `.cwd(dir)` 覆盖与会话级 `ShellDirState`（`~` / `~/sub` 家目录展开、`-` 与 `OLDPWD` 往返切换及词法规范化），避免 `process.chdir()` 污染多线程进程全局状态；大输出命令推荐逐行流式消费（对应 Bun `.lines()`）并显式检查 `exit_code`。
6. **POSIX `touch` 选项解析与时间戳校验（`lib/shell/touchOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/touch.cp`）**：
   - 零堆分配支持 `-a`、`-m`、`-c`/`--no-create`、`-h`/`--no-dereference`、`-d`/`--date`、`-r`/`--reference`、`-t [[CC]YY]MMDDhhmm[.ss]`、`--time=access|atime|use|modify|mtime`、短选项聚合（如 `-acm`）与 `--` 终止符；
   - `TouchTimestamp.parse()` 遵循 POSIX 两位年份窗规则（`69..99 => 1969..1999`，`00..68 => 2000..2068`）、闰年 2 月天数校验与闰秒 `60` 支持；互斥时间源（`-d`/`-r`/`-t` 混用）自动抛出 `ShellTouchError.ConflictingTimeSource`。
7. **POSIX `cp` 选项解析、符号链接策略与自拷贝环路防线（`lib/shell/cpOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/cp.cp`）**：
   - 零堆分配支持 `-r`/`-R`/`--recursive`、`-f`/`-i`/`-n`（后出现覆盖前者）、`-p`/`--preserve`、`-v`/`--verbose`、`-a`/`--archive`（等价 `-RPp`）与 `-H`/`-L`/`-P` 符号链接解析策略；
   - `CpPlan.resolveTargetFor()` 强制校验目录源必须开启递归标志（否则返回 `OmittingDirectory`）、多源或尾斜杠目标必须为目录（`DestinationMustBeDir`），并自动阻断自拷贝及目录递归拷贝至自身子目录（`CannotCopyIntoSelf`）。
8. **POSIX `rm` 选项解析与根目录/`.` 防误删守卫（`lib/shell/rmOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/rm.cp` & `/data/dev/bun/docs/runtime/shell.mdx`）**：
   - 零堆分配支持 `-f`/`--force`、`-r`/`-R`/`--recursive`（自动隐含 `-d`）、`-d`/`--dir`、`-v`/`--verbose`、`-i`/`-I`/`--interactive=never|once|always`、`--preserve-root`/`--no-preserve-root`、短标志聚合（如 `-rfv`）与 `--` 操作数终止哨兵（支持删除 `-weird` 开头文件名）；
   - **安全删除防线**：默认启用 `preserve_root = true`，`isRmRootPath(cwd, target)` 结合当前工作目录词法规范化折叠 `..` 后拦截 `/` 及 Windows 驱动器根 `C:/`（返回 `RefusingToRemoveRoot`）；`isRmDotOrDotDotTarget()` 严格拦截 `.` / `..` 尾段（返回 `RefusingToRemoveDotOrDotDot`）；`rm -f` 无操作数时按 POSIX 约定静默成功。
9. **POSIX/GNU `cat` 选项解析与流式行格式化器（`lib/shell/catOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/cat.cp` & `/data/dev/bun/docs/runtime/shell.mdx`）**：
   - 零堆分配支持 `-b`/`--number-nonblank`、`-n`/`--number`（`-b` 优先覆盖 `-n`）、`-s`/`--squeeze-blank`、`-E`/`--show-ends`、`-T`/`--show-tabs`、`-v`/`--show-nonprinting`、`-e`（`-vE`）、`-t`（`-vT`）、`-A`/`--show-all`（`-vET`）、`-u`、`--` 终止哨兵与 `-` 标准输入操作数（无操作数默认读取 stdin）；同时提供 `CatPlan.parseWithMode(args, true)` 对齐 Bun 当前严格兼容模式；
   - `CatLineFormatter` 为有状态流式格式化器，可在管道或分块读取（如 `cat < response | wc -c` 跨 chunk 场景）中保持连续行号与连续空行压缩状态，无内容改写选项时自动走零变换快速拷贝路径。
10. **POSIX/GNU `ls` 选项解析、权限/尺寸格式化与原地稳定排序器（`lib/shell/lsOpts.cp`，吸收自 `/data/dev/bun/src/shell/builtin/ls.cp` & `/data/dev/bun/docs/runtime/shell.mdx`）**：
    - 零堆分配支持 `-a`/`-A`/`-B`/`-d`/`-f`/`-F`/`-h`/`-l`/`-1`/`-p`/`-r`/`-R`/`-S`/`-t`/`-U` 短标志聚合、`--all`/`--almost-all`/`--ignore-backups`/`--group-directories-first` 等长选项与 `--` 终止符（无路径操作数默认列出 `"."`）；
    - `formatLsMode` 支持全部 7 类文件首字符与 setuid/setgid/sticky 特殊权限位；`formatLsHumanSize` 纯整数实现 `K`/`M`/`G`/`T`/`P` 尺寸格式化；`sortLsEntries` 原地稳定排序支持目录优先分组、大小/修改时间/名称排序及 `-r` 反转。
11. **跨平台 `which` 路径扫描器与退出码评估（`lib/shell/which.cp`，吸收自 `/data/dev/bun/src/shell/builtin/which.cp` & `/data/dev/bun/docs/runtime/shell.mdx`）**：
    - 零堆分配提供 `PathEnvIter`（POSIX `:` / Windows `;` 分隔，自动将空段规范化为当前目录 `.`）、`WhichPlan`（支持 `-a`/`-s`/`--` 与多命令顺序解析）、`resolveWhichSingle`（含路径分隔符命令跳过 `PATH` 直接相对 `cwd` 解析）及 `evaluateWhichExitCode`（空参数或任一命令未命中均返回退出码 `1`，全部命中返回 `0`）。
12. **Shell 管道拓扑规划器与执行状态机（`lib/shell/pipePlan.cp`，吸收自 `/data/dev/bun/src/shell/states/Pipeline.cp` & `/data/dev/bun/docs/runtime/shell.mdx`）**：
    - **零堆分配 `N` 阶段 `N-1` 管道布线**：自动跳过管道中的纯赋值项（`PipeStageKind.Assigns`），首阶段继承 `stdin`、末阶段继承 `stdout`、中间阶段按 `pipe[i-1]` 读 / `pipe[i]` 写串联；
    - **生命周期与防 FD 泄漏**：严格遵循 `StartingCmds -> Pending -> Done` 状态机，支持任意乱序子阶段完成（`onStageDone`），并在中途子进程创建失败时通过 `computeAbortCleanup(failed_stage_idx)` 计算精确的未移交管道读/写端清理区间；
    - **退出码策略与管道取反（参考 `/data/dev/bun/docs/runtime/shell.mdx`）**：默认 POSIX `PipeExitPolicy.LastStage` 取最后阶段退出码，`PipeExitPolicy.Pipefail` 取最右侧非零退出码，配合 `negated = true`（`! cmd1 | cmd2`）实现 `0 <-> 1` 逻辑取反；配合上层 `.nothrow()` 可在不抛出异常的前提下显式检查 `final_exit_code`。

### POSIX `rm` 选项解析与根目录防误删示例（`RmPlan`）

```cp
import { RmPlan, RmPathKind, RmTargetDecision } from "cap:shell";

const args: [3]([]const u8) = ["-rfv", "--", "-temp.log"];
const plan = RmPlan.parse(args[0..])?;
const decision = plan.validateTarget("/workspace/app", "-temp.log", RmPathKind.File)?;
// decision == RmTargetDecision.Remove
// plan.validateTarget("/workspace/app", "../..", RmPathKind.NonEmptyDir) -> ShellRmError.RefusingToRemoveRoot
* = decision;
```

### POSIX `cat` 选项解析与流式行格式化示例（`CatPlan` / `CatLineFormatter`）

```cp
import { CatPlan, CatLineFormatter } from "cap:shell";

const args: [3]([]const u8) = ["-bsE", "a.txt", "-"];
const plan = CatPlan.parse(args[0..])?;
let fmt = CatLineFormatter.init(plan.opts);
let out: [128]u8 = [0; 128];
const n = fmt.formatChunk("hello\n\n\nworld\n", out[0..])?;
// 输出: "     1\thello$\n$\n     2\tworld$\n"
* = n;
```

### Shell 管道拓扑布线与乱序退出状态机示例（`ShellPipelinePlanner`）

```cp
import {
    PipeStageKind,
    PipeExitPolicy,
    ShellPipelinePlanner,
} from "cap:shell";

let pp = ShellPipelinePlanner.init(false, PipeExitPolicy.Pipefail);
* = pp.addItem(PipeStageKind.Cmd, "cat")?;
* = pp.addItem(PipeStageKind.Cmd, "grep")?;
* = pp.addItem(PipeStageKind.Cmd, "wc")?;
pp.finalizeWiring();

while (sid := pp.nextStageToStart()) {
    * = sid;
}
* = pp.onStageDone(2, 0)?; // wc 先退出
* = pp.onStageDone(0, 0)?; // cat 退出
const done = pp.onStageDone(1, 2)?; // grep 退出码 2 -> Pipefail 汇总为 2
// done == true, pp.final_exit_code == 2
* = done;
```

### POSIX / Bun Shell `yes` 高速 8KiB 块倍增生成器与协同让出状态机（`yesStream.cp`）

- 吸收自 `/data/dev/bun/src/shell/builtin/yes.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`（跨平台内建 `yes` 命令）；
- 导出：`YES_BUFSIZ` (`8192`)、`YES_HALF_BUFSIZ` (`4096`)、`YES_CHUNKS_PER_TICK` (`4`)、`ShellYesError`、`YesState`、`YesTickAction`、`YesLayout`、`computeYesLineLen`、`computeYesLayout`、`fillYesChunk`、`YesStreamState`；
- **最佳实践与边界约束**：
  - 无参数默认输出 `"y\n"`，多参数以单空格连接并追加 `'\n'`；
  - 单行长度 `<= 4096` 时按整行倍增平铺填满 `8192` 字节块（`copies = buf_cap / line_len`），**绝不截断半行**；
  - 同步管道模式下每轮最多连续写出 `4` 个 8KiB 块（`32KiB`）后自动返回 `YesTickAction.YieldToEventLoop`，避免无限输出阻塞事件循环；
  - 下游管道关闭（如 `yes | head -n 5` 触发 `EPIPE`）可通过 `onWriteError(true)` 优雅收敛为 `YesState.Done`（退出码 `0`），其他 I/O 错误返回退出码 `1`。

```cp
import { YesStreamState, YesTickAction } from "cap:shell";

const args: [2]([]const u8) = ["hello", "world"];
let ys = YesStreamState.init(args[0..])?;
// ys.lineLen() == 12 ("hello world\n"), ys.copiesPerChunk() == 682, ys.chunkSlice().len == 8184
while (ys.nextStep() == YesTickAction.WriteChunk) {
    ys.onChunkWritten(ys.chunkSlice().len);
}
// 连续写出 4 块 (32KiB) 后自动进入 Suspended 并让出事件循环
* = ys.resumeFromEventLoop();
* = ys.onWriteError(true); // EPIPE -> exit_code == 0
```

### POSIX / Bun Shell `if / elif / else` 多分支控制流状态机（`ifState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/If.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_IF_MAX_STMTS` (`32`)、`SHELL_IF_MAX_ELSE_PARTS` (`16`)、`ShellIfError`、`IfBranchPhase`（`Idle`/`Cond`/`Then`/`ElifCond`/`ElseBranch`/`Done`）、`IfStepKind`（`RunStmt`/`Completed`）、`IfStepResult`、`ShellIfStateMachine`；
- **POSIX 2.9.4.4 与 Bun 扁平 `else_parts` 约束**：
  - 多语句条件列表顺序求值，**仅依据最后一条语句的退出码**（`== 0` 为真）决定是否进入 `then` 分支；
  - 扁平 `else_parts` 编码：`len == 0` 表示无 `elif`/`else`；`len == 1` 表示单 `else` 块；`len >= 2` 时偶数槽为 `elif` 条件、奇数槽为对应 `then` 体，长度为奇数时末槽为兜底 `else`；
  - 当所有条件均为假且无 `else` 时，整个 `if` 控制流严格返回退出码 `0`；命中 `then` 或 `else` 分支时返回该分支最后一条语句的退出码。

```cp
import { ShellIfStateMachine, IfStepKind } from "cap:shell";

let sm = ShellIfStateMachine.init();
* = sm.addCondStmt(10)?;
* = sm.addThenStmt(20)?;
const p_else = sm.beginElsePart()?;
* = sm.addElsePartStmt(p_else, 30)?;

const s1 = sm.nextStep()?; // RunStmt(10)
* = sm.onStmtDone(1)?;     // 条件为假 -> 跳转 else 分支
const s2 = sm.nextStep()?; // RunStmt(30)
* = sm.onStmtDone(42)?;
const s3 = sm.nextStep()?; // Completed(exit_code == 42)
* = s1;
* = s2;
* = s3;
```

### POSIX / Bun Shell `&&` / `||` 短路二元逻辑链状态机（`binaryState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Binary.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_BINARY_MAX_TERMS` (`32`)、`ShellBinaryError`、`BinaryLogicOp`（`None`/`And`/`Or`）、`BinaryStepKind`（`RunTerm`/`Completed`）、`BinaryTerm`、`BinaryStepResult`、`shouldShortCircuitBinary`、`ShellBinaryStateMachine`；
- **POSIX AND-OR 列表规范与边界约束**：
  - `&&` 与 `||` 具有**相同优先级且严格左结合**（`cmd1 && cmd2 || cmd3 && cmd4` 按 `(((cmd1 && cmd2) || cmd3) && cmd4)` 顺序求值）；
  - `&&` 在前序退出码 `!= 0` 时短路跳过右操作数，`||` 在前序退出码 `== 0` 时短路跳过右操作数；
  - 对齐 Bun `Binary.makeChild == null` 语义：空子节点（`is_empty = true`）不派发执行并直接以退出码 `0` 继续推进。

```cp
import {
    BinaryLogicOp,
    BinaryStepKind,
    ShellBinaryStateMachine,
} from "cap:shell";

let bm = ShellBinaryStateMachine.init();
* = bm.setFirstTerm(10, false)?;                    // cmd10 (将返回 1)
* = bm.pushTerm(BinaryLogicOp.And, 20, false)?;     // && cmd20 (被短路跳过)
* = bm.pushTerm(BinaryLogicOp.Or, 30, false)?;      // || cmd30 (执行并返回 0)
* = bm.pushTerm(BinaryLogicOp.And, 40, false)?;     // && cmd40 (执行并返回 0)

const r1 = bm.nextStep()?; // RunTerm(10)
* = bm.onTermDone(1)?;
const r2 = bm.nextStep()?; // RunTerm(30), skipped_count == 1
* = bm.onTermDone(0)?;
const r3 = bm.nextStep()?; // RunTerm(40)
* = bm.onTermDone(0)?;
const done = bm.nextStep()?; // Completed(exit_code == 0)
* = r1;
* = r2;
* = r3;
* = done;
```

### POSIX / Bun Shell 环境变量赋值展开与作用域提交状态机（`assigns.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Assigns.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_ASSIGNS_MAX_ITEMS` (`16`)、`SHELL_ASSIGN_MAX_LABEL_LEN` (`64`)、`SHELL_ASSIGN_MAX_VALUE_LEN` (`256`)、`ShellAssignsError`、`AssignScopeCtx`（`Cmd`/`Shell`/`Exported`）、`AssignStepKind`（`ExpandValue`/`Completed`/`Failed`）、`AssignStepResult`、`CommittedAssign`、`isValidAssignLabel`、`canSkipPipelineAssigns`、`joinExpandedWords`、`ShellAssignsStateMachine`；
- **POSIX 赋值与管道优化约束**：
  - 区分 `Cmd`（单命令前缀作用域）、`Shell`（当前 Shell 局部作用域）与 `Exported`（`export` 导出作用域）；
  - 管道中的独立 `Shell`/`Exported` 赋值（如 `FOO=bar | echo hi`）对父 Shell 环境无副作用，`canSkipPipelineAssigns` 返回 `true` 直接跳过；
  - 右侧展开产生多个词时以单空格拼接（`0 词 -> ""`），同名变量按自左向右 last-write-wins 覆盖；任意项展开失败时立即中止并返回退出码 `1`。

```cp
import { AssignScopeCtx, AssignStepKind, ShellAssignsStateMachine } from "cap:shell";

let am = ShellAssignsStateMachine.init(AssignScopeCtx.Exported, false);
* = am.addAssignLabel("NODE_ENV")?;
* = am.addAssignLabel("FLAGS")?;

const s1 = am.nextStep()?; // ExpandValue(0)
const w1: [1]([]const u8) = ["production"];
* = am.onExpansionDone(w1[0..])?;

const s2 = am.nextStep()?; // ExpandValue(1)
const w2: [2]([]const u8) = ["--fast", "--strict"];
* = am.onExpansionDone(w2[0..])?;

const done = am.nextStep()?; // Completed(exit_code == 0), FLAGS == "--fast --strict"
* = s1;
* = s2;
* = done;
```

### POSIX / Bun Shell 顶层脚本与语句序列状态机（`scriptState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Script.cp`、`Stmt.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_SCRIPT_MAX_EXPRS` (`64`)、`ShellScriptError`、`ScriptHostKind`（`TopLevel`/`Subshell`/`CommandSubstitution`）、`ShellExprKind`（8 种表达式）、`ScriptStepKind`（`RunExpr`/`Completed`/`AbortedOnError`）、`ScriptExprItem`、`ScriptStepResult`、`scriptPropagatesEnvToParent`、`ShellScriptStateMachine`；
- **执行与错误传播约束**：
  - 空脚本（0 条语句）立即以退出码 `0` 结束；
  - 支持 Bun Shell 默认 `errexit`（`.throws(true)`）遇非零退出码立即中止（`AbortedOnError`）与 `nothrow` 顺序执行模式；
  - 对条件语句等豁免项（`exempt_from_errexit = true`）不触发 `errexit` 提前中止；仅 `TopLevel` 宿主向外传播环境副作用，`Subshell` 与 `CommandSubstitution` 保持环境隔离。

```cp
import {
    ScriptHostKind,
    ShellExprKind,
    ScriptStepKind,
    ShellScriptStateMachine,
} from "cap:shell";

let sc = ShellScriptStateMachine.init(ScriptHostKind.TopLevel, true);
* = sc.pushExpr(ShellExprKind.Assign, 1, false)?;
* = sc.pushExpr(ShellExprKind.Cmd, 2, false)?;
* = sc.pushExpr(ShellExprKind.Cmd, 3, false)?;

* = sc.nextStep()?; // RunExpr(1)
* = sc.onExprDone(0)?;
* = sc.nextStep()?; // RunExpr(2)
const cont = sc.onExprDone(127)?; // false (触发 errexit 中止)
const end_s = sc.nextStep()?;     // AbortedOnError(exit_code == 127), 语句 3 不会执行
* = cont;
* = end_s;
```

### POSIX / Bun Shell 子 Shell 隔离与重定向状态机（`subshellState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Subshell.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SUBSHELL_MAX_REDIRECT_PATH_LEN` (`512`)、`SUBSHELL_MAX_ENV_SNAPSHOT` (`32`)、`SubshellError`、`SubshellParentKind`（`Pipeline`/`Binary`/`Stmt`）、`SubshellRunState`（`Idle`/`ExpandingRedirect`/`Exec`/`WaitWriteErr`/`Done`）、`SubshellRedirectKind`（`None`/`WriteTrunc`/`WriteAppend`/`Read`/`ErrTrunc`/`ErrAppend`/`BothTrunc`）、`SubshellExecPolicy`、`SubshellEnvEntry`、`SubshellStepKind`、`SubshellStepAction`、`SubshellStateMachine`；
- **Bun Shell 隔离与错误处理最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx`）**：
  - 子 Shell `( ... )` 在启动时复制父 Shell 环境变量与工作目录快照（`captureParentEnv`），内部通过 `setLocalEnv()` / `setLocalCwd()` 所做的任何修改均严格隔离在子 Shell 空间内（`propagatesSideEffectsToParent() == false`），绝不反向污染父 Shell；
  - 带外层重定向（如 `(echo hi) > out.txt`）时状态机严格走 `Idle -> ExpandingRedirect -> Exec -> Done` 路径，先校验重定向路径无控制字符且长度 `<= 512`；重定向失败时走 `WaitWriteErr -> Done`（启用 `.quiet()` 时跳过错误回显直接 `Done`）；
  - 默认 `throw_on_nonzero = true`（对齐 `$` 默认非零退出码抛出 `ShellError`），可通过 `SubshellExecPolicy{ throw_on_nonzero: false }`（对应 `.nothrow()`）改为手动检查 `exitCode()`。

```cp
import {
    SubshellParentKind,
    SubshellRedirectKind,
    SubshellExecPolicy,
    SubshellStateMachine,
} from "cap:shell";

let sub = SubshellStateMachine.init(
    SubshellParentKind.Stmt,
    SubshellRedirectKind.WriteTrunc,
    SubshellExecPolicy{ throw_on_nonzero: false },
);
* = sub.captureParentEnv("PATH", "/usr/bin")?;
* = sub.setLocalEnv("FOO", "bar")?; // 仅子 Shell 可见
* = sub.nextStep()?;                // ExpandRedirect
* = sub.onRedirectExpanded("/tmp/out.txt")?;
* = sub.nextStep()?;                // ExecuteScript
* = sub.onScriptFinished(0)?;
const done = sub.nextStep()?;       // Completed, exit_code == 0
* = done;
```

### POSIX / Bun Shell 后台异步 (`&`) 作业状态机与排空追踪器（`asyncState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Async.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_ASYNC_MAX_JOBS` (`32`)、`ShellAsyncError`、`AsyncParentKind`（`Binary`/`Stmt`）、`AsyncChildKind`（`Pipeline`/`Cmd`/`If`/`CondExpr`）、`AsyncRunPhase`（`Idle`/`Scheduled`/`Executing`/`Done`）、`AsyncStepKind`（`EnqueueAndReturnParentZero`/`StartChild`/`Suspended`/`Finished`）、`AsyncStepAction`、`ShellAsyncStateMachine`（`init`/`start`/`onEventLoopTick`/`onChildDone`/`isInFlight`）、`ShellAsyncJobTracker`（`init`/`spawn`/`tickJob`/`finishJob`/`inFlightCount`/`canFinishShell`）；
- **后台异步任务排空与非阻塞语义最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx` & `Async.cp`）**：
  - 前台遇到 `cmd &` 时，`start()` 立即将后台作业入队事件循环并向父状态返回退出码 `0`（不阻塞前台语句继续执行），后台执行严格按 `Idle -> Scheduled -> Executing -> Done` 推进；
  - 解释器通过 `ShellAsyncJobTracker.inFlightCount()`（对应 Bun `async_commands_executing`）追踪所有在途后台作业；即使前台脚本已结束，也必须等待 `canFinishShell() == true`（所有后台 `&` 作业均已排空）后再释放解释器上下文；已完成的槽位自动回收复用。

```cp
import { AsyncParentKind, AsyncChildKind, ShellAsyncJobTracker } from "cap:shell";

let tracker = ShellAsyncJobTracker.init();
const j = tracker.spawn(AsyncParentKind.Stmt, AsyncChildKind.Pipeline, 10)?;
// j.parent_exit_code == 0, tracker.canFinishShell() == false
* = tracker.tickJob(j.job_id)?;
* = tracker.finishJob(j.job_id, 0)?;
// tracker.canFinishShell() == true
```

### Shell 三通道 IO 描述符、引用计数句柄表与子进程 Stdio 路由器（`ioChannels.cp`）

- 吸收自 `/data/dev/bun/src/shell/IO.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`SHELL_IO_MAX_HANDLES` (`64`)、`SHELL_IO_INVALID_HANDLE`、`ShellIoError`、`ShellInKind`（`Ignore`/`Fd`）、`ShellOutKind`（`Ignore`/`Pipe`/`Fd`）、`SubprocStdioKind`（`Ignore`/`Pipe`/`Inherit`/`Fd`/`Capture`）、`SubprocStdioRoute`、`ShellSubprocStdioPlan`、`ShellInChannel`、`ShellOutChannel`、`ShellIoTriplet`（`ignored`/`quietPipe`）、`ShellIoHandleEntry`、`ShellIoHandleTable`（`init`/`openHandle`/`getHandle`/`refHandle`/`derefHandle`/`copyTriplet`/`releaseTriplet`/`toSubprocStdio`/`estimateMemoryCost`/`formatTriplet`）；
- **`.quiet()` 管道捕获与跨平台 Stdio 路由最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx` & `IO.cp`）**：
  - **`.quiet()` 与 `.text()` 静默缓冲**：Bun Shell 默认将命令输出直通终端；调用 `.quiet()`（或隐式启用 `.quiet()` 的 `.text()`）时将 `stdout`/`stderr` 置为 `ShellOutKind.Pipe`（对应 `ShellIoTriplet.quietPipe()`）；
  - **输出通道三态路由**：`toSubprocStdio()` 对 `.Fd` 通道按优先级判定：① 带捕获缓冲（`has_capture == true`）路由至 `.Capture`（同时写回流与缓冲）；② 有效 OS fd 且未移交事件循环路由至 `.Fd`；③ 已被事件循环接管（如 Windows/libuv）则安全回退至 `.Inherit`；
  - **引用计数生命周期防泄漏**：管道/子 Shell 共享 IO 通道时必须通过 `copyTriplet()` 递增引用、退出时通过 `releaseTriplet()` 递减，归零自动关闭并拦截双重释放（`AlreadyClosed`）。

```cp
import { ShellIoHandleTable, ShellIoTriplet, ShellInChannel, ShellOutChannel } from "cap:shell";

let tbl = ShellIoHandleTable.init();
const h_out = tbl.openHandle(1, false, 128)?;
const io = ShellIoTriplet{
    stdin: ShellInChannel{ kind: .Ignore },
    stdout: ShellOutChannel{ kind: .Fd, handle: h_out, has_capture: true, capture_buf_id: 7 },
    stderr: ShellOutChannel{ kind: .Pipe },
};
const plan = tbl.toSubprocStdio(io)?; // stdout -> .Capture(buf=7), stderr -> .Pipe
* = plan;
```

### Shell 混合借用/引用计数环境字符串与共享池（`envStr.cp`）

- 吸收自 `/data/dev/bun/src/shell/EnvStr.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`ENV_STR_MAX_SLOTS` (`64`)、`ENV_STR_ARENA_CAP` (`4096`)、`ENV_STR_INVALID_SLOT`、`EnvStrError`、`EnvStrTag`（`Empty`/`Slice`/`RefCounted`）、`EnvStr`（`empty`/`initSlice`/`isEmpty`/`isRefCounted`）、`EnvStrSlot`、`EnvStrPool`（`init`/`dupeRefCounted`/`slice`/`ref`/`deref`/`refCount`/`memoryCost`/`eql`）；
- **环境变量高频拷贝与内存分摊最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx` & `EnvStr.cp`）**：
  - **三态零开销折叠**：空串统一折叠为 `.Empty`（不消耗池槽位）；来自 AST 或编译期常量的静态环境值使用 `.Slice`（零拷贝、零引用计数）；动态值使用 `.RefCounted`，在管道与子 Shell 复制 `.env(...)` 空间时仅递增引用计数而不深拷贝字节载荷；
  - **GC 均摊内存统计**：`memoryCost()` 按 `len / ref_count` 均摊共享字符串内存，避免多子 Shell 重复累计同一环境变量字节数；归零后自动拦截二次释放（`AlreadyReleased`）与悬垂读取。

```cp
import { EnvStr, EnvStrPool } from "cap:shell";

let pool = EnvStrPool.init();
const s_static = EnvStr.initSlice("/usr/local/bin");
const s_rc = pool.dupeRefCounted("/usr/local/bin")?;
* = pool.ref(s_rc)?;                     // ref_count == 2
const cost = pool.memoryCost(s_rc);      // 14 / 2 == 7
const same = pool.eql(s_static, s_rc)?;  // true
* = cost;
* = same;
```

### Shell 跨平台引用计数环境变量映射表（`envMap.cp`）

- 吸收自 `/data/dev/bun/src/shell/EnvMap.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`ENV_MAP_MAX_ENTRIES` (`64`)、`EnvMapError`、`EnvMapEntry`、`ShellEnvMap`（`init`/`len`/`slice`/`insert`/`get`/`getSlice`/`remove`/`clear`/`clone`/`memoryCost`/`formatSortedExports`）；
- **跨平台环境变量大小写与子 Shell 隔离最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx` & `EnvMap.cp`）**：
  - **POSIX vs Windows 键匹配**：POSIX 下 `PATH` 与 `Path` 为不同键（`case_insensitive_keys = false`）；Windows 下环境变量名 ASCII 大小写不敏感（`case_insensitive_keys = true`，写入 `Path` 将覆盖已有 `PATH`）；
  - **零拷贝克隆与引用计数闭环**：子 Shell/管道通过 `clone(&pool)` 仅递增引用计数而不复制字符串字节；覆盖写入或删除时自动递减旧值引用计数，退出时调用 `clear(&pool)` 确保零泄漏；
  - **确定性字典序导出**：`formatSortedExports()` 使用零堆分配稳定排序按 ASCII 字典序输出 `KEY=VALUE\n`，对齐 `export` 无参输出规范。

```cp
import { EnvStr, EnvStrPool, ShellEnvMap } from "cap:shell";

let pool = EnvStrPool.init();
let env = ShellEnvMap.init(true); // Windows ASCII case-insensitive mode
const k1 = pool.dupeRefCounted("PATH")?;
const v1 = pool.dupeRefCounted("/usr/bin")?;
* = env.insert(&pool, k1, v1)?;
let child = env.clone(&pool)?; // zero-copy refcounted clone
const got = child.getSlice(&pool, "path")?; // "/usr/bin"
* = got;
```

### Shell 词展开、命令替换分词与波浪号/Glob 步进引擎（`wordExpand.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Expansion.cp` 与 `/data/dev/bun/docs/runtime/shell.mdx`；
- 导出：`WORD_EXPAND_MAX_ATOMS` (`32`)、`WORD_EXPAND_MAX_WORDS` (`32`)、`WORD_EXPAND_ARENA_CAP` (`2048`)、`WORD_EXPAND_MAX_VARS` (`32`)、`WordExpandError`、`WordExpandMode`（`Single`/`Array`）、`WordAtomKind`（`Literal`/`QuotedLiteral`/`Var`/`QuotedVar`/`Tilde`/`CmdSubst`/`QuotedCmdSubst`/`GlobPattern`）、`WordExpandStepKind`（`NeedCmdSubst`/`NeedGlobWalk`/`Done`）、`WordVarBinding`、`WordAtom`、`WordExpandStep`、`WordSpan`、`WordExpandOutput`、`stripTrailingNewlines`、`hasUnquotedGlobMeta`、`normalizeUnquotedSubshellStdout`、`expandLeadingTilde`、`ShellWordExpander`（`init`/`setVar`/`lookupVar`/`pushAtom`/`step`/`onCmdSubstDone`/`onGlobDone`/`pendingGlobPattern`/`isDone`）；
- **Shell 词展开与防注入最佳实践（参考 `/data/dev/bun/docs/runtime/shell.mdx` & `Expansion.cp`）**：
  - **引号空串保留 vs 未引号空变量省略**：`echo ""` 或 `"$EMPTY"` 必须设置 `has_quoted_empty` 并产生 1 个空参数，而未引号的 `$EMPTY` 展开后不产生任何参数；
  - **命令替换 `$(...)` vs `"$(...)"`**：双引号内仅剥离末尾所有连续 `\n` 不分词；未引号模式下在 `Array` 参数上下文中将内部换行折叠为空格、修剪首尾空白并按空白拆分为多个参数（与 Bun `postSubshellExpansion` 完全一致）；
  - **前导波浪号与 Glob 回退**：仅当 `~` 位于词首且后续为空或 `/` / `\` 时展开为 `$HOME`（`~user` 保持原样）；未引号通配符触发 `.NeedGlobWalk`，若宿主无匹配文件则按 POSIX 规范回退保留原始字面量。

```cp
import { ShellWordExpander, WordExpandOutput, WordAtom } from "cap:shell";

let out = WordExpandOutput.init();
let exp = ShellWordExpander.init(.Array, "/home/alice");
* = exp.pushAtom(WordAtom{ kind: .Tilde })?;
* = exp.pushAtom(WordAtom{ kind: .Literal, text: "/docs" })?;
const st = exp.step(*out)?; // .Done, out.wordAt(0) == "/home/alice/docs"
* = st;
```

### POSIX `basename` / `dirname` 路径内建命令（`pathBuiltin.cp`）

- 吸收自 `/data/dev/bun/src/shell/builtin/basename.cp`（参考 `/data/dev/bun/docs/runtime/shell.mdx` Builtin Commands）；
- 导出：`SHELL_PATH_BUILTIN_MAX_ARGS` (`64`)、`PathBuiltinError`、`BasenameOpts`、`DirnameOpts`、`posixBasename`、`posixBasenameStripSuffix`、`posixDirname`、`runBasenameBuiltin`、`runDirnameBuiltin`；
- **POSIX 规范与零分配契约**：
  - `posixBasename` / `posixDirname` 直接返回输入切片的子视图（零堆分配），严格处理空路径（`dirname("") == "."`）、全斜杠（`basename("///") == "/"`、`dirname("///") == "/"`）与尾随斜杠剥离；
  - `posixBasenameStripSuffix` 在后缀与完整文件名相等时按 POSIX 规范保留完整文件名；
  - `runBasenameBuiltin` / `runDirnameBuiltin` 支持 `-a`/`--multiple`、`-s SUFFIX`/`--suffix=`、`-z`/`--zero`（NUL 分隔，适合管道配合 `xargs -0` 处理含空白路径）与 `--` 选项终止符。

```cp
import { runBasenameBuiltin, runDirnameBuiltin } from "cap:shell";

let out: [128]u8 = [0; 128];
const b_args: [4][]const u8 = ["-s", ".cp", "lib/a.cp", "lib/b.cp"];
const n1 = runBasenameBuiltin(b_args[0..], out[0..])?; // "a\nb\n"
const d_args: [1][]const u8 = ["/usr/local/bin/"];
const n2 = runDirnameBuiltin(d_args[0..], out[0..])?;  // "/usr/local\n"
* = n1; * = n2;
```

### POSIX `export` 内建命令与零分配环境变量表（`exportBuiltin.cp`）

- 吸收自 `/data/dev/bun/src/shell/builtin/export.cp` & `/data/dev/bun/docs/runtime/shell.mdx`（Environment variables 章节）；
- 导出：`SHELL_EXPORT_MAX_VARS` (`64`)、`ShellExportError`、`isValidShellIdent`、`ShellExportEntry`、`ShellExportTable`（`init`/`setVar`/`applyArg`/`applyArgs`/`get`/`isExported`/`unset`/`count`/`exportedCount`/`formatSorted`）；
- **Bun Shell 环境变量与 `export` 实践**：
  - 支持行内赋值（`FOO=bar cmd`）、局部/全局 `.env()` 覆盖与内置 `export` 命令；
  - `applyArg` 严格按 POSIX 规则校验变量名 `[A-Za-z_][A-Za-z0-9_]*`（拒绝数字开头、连字符或空键），对 `NAME=a=b` 仅在首个 `=` 处切分；对裸 `NAME` 优先将已有局部变量提升为已导出变量，否则以空串导出；
  - 无参 `export`（`formatSorted`）按 key ASCII 字典序升序排序并零分配格式化为 `"KEY=VALUE\n"`，忽略未导出的局部变量。

```cp
import { ShellExportTable } from "cap:shell";

let env = ShellExportTable.init();
* = env.setVar("LOCAL_ONLY", "secret", false)?;
const args: [3][]const u8 = ["Z_KEY=last", "A_KEY=first=1", "B_EMPTY"];
* = env.applyArgs(args[0..])?;
let buf: [128]u8 = [0; 128];
const n = env.formatSorted(buf[0..])?;
// "A_KEY=first=1\nB_EMPTY=\nZ_KEY=last\n"
* = n;
```

### POSIX `pwd` 内建命令状态机与 `-L`/`-P` 解析器（`pwdBuiltin.cp`）

- 吸收自 `/data/dev/bun/src/shell/builtin/pwd.cp`；
- 导出：`PwdOutputKind`（`Stdout` / `Stderr`）、`PwdState`（`Idle` / `WaitingStdout` / `WaitingStderr` / `Err` / `Done`）、`PwdMode`（`Logical` / `Physical`）、`PwdError`（`TooManyArguments` / `InvalidOption` / `BufferTooSmall` / `InvalidState`）、`PwdResult`、`parsePwdArgs`、`PwdBuiltin`（`init` / `isTerminal` / `start` / `startAsync` / `onIoComplete`）、`runPwdBuiltin`；
- 支持 POSIX `-L`（逻辑路径）/ `-P`（物理路径）及组合标志 `-LP`/`-PL`（最后一个生效）与 `--` 选项结束符，位置参数过多时按 Bun 规范输出 `"pwd: too many arguments\n"` 并返回退出码 `1`，零堆分配写入调用方缓冲区。

```cp
import { PwdBuiltin, PwdMode } from "cap:shell";

let pwd = PwdBuiltin.init();
const args: [1][]const u8 = ["-P"];
let out: [128]u8 = [0; 128];
const res = pwd.start(args[0..], "/home/user/link", "/data/real", out[0..])?;
// out[0..res.bytes_written] == "/data/real\n", res.exit_code == 0, res.mode == PwdMode.Physical
* = res;
```

### Shell 语句子表达式顺序分派状态机（`stmtState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Stmt.cp`；
- 导出：`SHELL_STMT_MAX_CHILDREN` (`32`)、`StmtParentKind`（`Script` / `If`）、`StmtChildKind`（`Async` / `Binary` / `Pipeline` / `Cmd` / `Assign` / `If` / `CondExpr` / `Subshell`）、`StmtError`（`TooManyChildren` / `ChildInFlight` / `NoChildInFlight` / `AlreadyFinished`）、`StmtAction`（`DispatchChild` / `Finish`）、`StmtStep`、`StmtState`（`init` / `pushChild` / `remaining` / `finalExitCode` / `next` / `childDone` / `reset`）；
- 精确建模 `Script` / `If` 父上下文下的多子表达式顺序调度：空语句直接返回 `Finish(exit_code=0)`，非空语句按序分派并在全部完成后传播最后一个子表达式的退出码，支持 `reset()` 零分配复用。

```cp
import { StmtState, StmtParentKind, StmtChildKind, StmtAction } from "cap:shell";

let st = StmtState.init(StmtParentKind.Script);
* = st.pushChild(StmtChildKind.Assign)?;
* = st.pushChild(StmtChildKind.Pipeline)?;
const s0 = st.next()?; // DispatchChild(0, Assign)
* = st.childDone(0)?;
const s1 = st.next()?; // DispatchChild(1, Pipeline)
* = st.childDone(2)?;
const s2 = st.next()?; // Finish(exit_code=2)
* = s0; * = s1; * = s2;
```

### POSIX `dirname` 内建命令状态机与零分配路径求值器（`dirnameBuiltin.cp`）

- 吸收自 `/data/dev/bun/src/shell/builtin/dirname.cp`；
- 导出：`DIRNAME_USAGE`、`DirnameOutputKind`（`Stdout` / `Stderr`）、`DirnameState`（`Idle` / `WaitingStdout` / `WaitingStderr` / `Err` / `Done`）、`DirnameError`（`BufferTooSmall` / `InvalidState`）、`DirnameResult`、`evalPosixDirname`、`DirnameBuiltin`（`init` / `isTerminal` / `start` / `startAsync` / `onIOWriterChunk`）、`runDirnameBuiltin`；
- 严格遵循 POSIX `dirname` 规范处理空串、全斜杠根路径、尾斜杠与多参数批量换行输出；零参数时输出 `usage: dirname path ...\n` 至 `Stderr` 并返回退出码 `1`，支持异步写回调 `onIOWriterChunk` 收敛终态。

```cp
import { runDirnameBuiltin } from "cap:shell";

const args: [2][]const u8 = ["/usr/local/bin/", "file.txt"];
let out: [64]u8 = [0; 64];
const res = runDirnameBuiltin(args[0..], out[0..])?;
// out[0..res.bytes_written] == "/usr/local\n.\n", res.exit_code == 0
* = res;
```

### Shell `Cmd` 七态执行状态机与管道 I/O 关闭栅栏（`cmdState.cp`）

- 吸收自 `/data/dev/bun/src/shell/states/Cmd.cp`；
- 导出：`CmdParentKind`（`Stmt` / `Binary` / `Pipeline` / `Async`）、`CmdExecKind`（`None` / `Builtin` / `Subprocess`）、`CmdPhase`（`Idle` / `ExpandingAssigns` / `ExpandingRedirect` / `ExpandingArgs` / `Exec` / `WaitingWriteErr` / `Done`）、`CmdIoChannel`（`Stdin` / `Stdout` / `Stderr`）、`CmdStepAction`、`CmdStateError`、`CmdStep`、`CmdBufferedIoTracker`、`CmdStateMachine`；
- 精确还原 Bun Shell 命令执行全生命周期：按 `Assigns -> Redirects -> Args -> Exec` 确定性推进；子进程模式下内置 `CmdBufferedIoTracker` 栅栏，严格保证仅在子进程退出且全部管道化的 `stdin`/`stdout`/`stderr` 关闭排空后才转入 `Done`，彻底避免输出截断与悬空管道引用。

```cp
import { CmdStateMachine, CmdParentKind, CmdIoChannel } from "cap:shell";

let cmd = CmdStateMachine.init(CmdParentKind.Pipeline, false, 0, 1);
* = cmd.next()?; // ExpandArg(0)
* = cmd.onArgDone()?;
* = cmd.next()?; // RunExec
* = cmd.startSubprocess(false, true, false)?;
* = cmd.onSubprocessExited(0)?; // false (等待 stdout 管道关闭)
const done = cmd.onSubprocessIoClosed(CmdIoChannel.Stdout)?; // true -> Done
* = done;
```

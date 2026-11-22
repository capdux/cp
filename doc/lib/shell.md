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
├── builtin_fs.cp    # 文件系统 Builtins (ls, cat, mkdir, rmdir, rm, cp, mv, touch, basename, dirname, mktemp)
├── builtin_text.cp  # 文本与流处理 Builtins (seq, head, tail, wc, grep, yes, sort, uniq)
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
- **`lib/shell/cond_expr.cp`**（吸收自 `/data/dev/bun/src/shell/states/CondExpr.cp`，零堆分配 Bash `[[ ... ]]` 条件表达式解析与求值器）：`MAX_COND_DEPTH`、`POSIX_S_*`、`CondExprError`、`CondOp`、`CondFileStat`、`CondStatFn`、`condToExitCode`、`CondExprEvaluator`（支持 `-e/-f/-d/-c/-L/-s/-r/-w/-x` 文件测试、`-z/-n/==/!=/</>` 字符串测试、`-eq/-ne/-lt/-le/-gt/-ge` 64 位整数比较、`&&`/`||`/`!`/`(...)` 逻辑组合与递归深度防护）
- **`lib/shell/echo.cp`**（吸收自 `/data/dev/bun/src/shell/builtin/echo.cp`，零堆分配 GNU/Bash `echo` 标志解析与反斜杠转义展开引擎）：`EchoError`、`EchoOptions`、`EchoEscapeResult`、`expandEchoEscapes`、`formatEchoArgs`（支持 `-n`/`-e`/`-E` 组合标志、`\\`/`\a`/`\b`/`\c`/`\e`/`\f`/`\n`/`\r`/`\t`/`\v`/`\0NNN`/`\xHH` 转义与 `\c` 提前截断）
- **`lib/shell/trampoline.cp`**（吸收自 `/data/dev/bun/src/shell/Yield.cp`，零堆分配栈安全 Shell Trampoline & Continuation 调度内核）：`MAX_TRAMPOLINE_REENTRY`、`MAX_PIPELINE_STACK_DEPTH`、`DEFAULT_MAX_TRAMPOLINE_STEPS`、`TrampolineError`、`YieldKind`、`PipelinePhase`、`ShellYield`、`PipelineStack`、`TrampolineResult`、`ShellTrampoline`（支持非递归状态机步进、嵌套管道栈自顶向下 `drainPipelines` 排空、`io_writer_chunk` 续跑、RAII `~init()` 重入深度守卫与步数上限防护）

### 栈安全 Trampoline & Continuation 调度示例（`trampoline.cp`）

```typescript
import { ShellTrampoline, ShellYield, PipelinePhase } from "cap:shell";

let tramp = ShellTrampoline.init();
const res = tramp.run(MyCtx, &ctx, ShellYield.forNode(.pipeline, 1), myStepFn, myPhaseFn)?;
// res.final_yield, res.steps_executed, res.pipelines_drained, res.max_pipeline_depth
```

- **`lib/shell/mkdir_opts.cp`**（吸收自 `/data/dev/bun/src/shell/builtin/mkdir.cp`，零堆分配 POSIX `mkdir` 选项解析与父目录规划器）：`DEFAULT_MKDIR_MODE` (`0o777`)、`MkdirParseError`、`parseOctalMode`、`MkdirOpts`（支持 `-p`/`--parents`、`-v`/`--verbose`、`-m`/`--mode` 组合短标志与 `--` 终结符）、`ParentDirIter`（零分配逐级父目录前缀迭代器）

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

### Bash `[[ ... ]]` 条件表达式求值示例（`cond_expr.cp`）

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
5. **重定向、管道与非零退出码治理（吸收自 `/data/dev/bun/docs/runtime/shell.mdx`）**：
   - 完整支持标准文件描述符重定向（`<`、`>`/`1>`、`2>`、`&>`、`>>`/`2>>`/`&>>`、`1>&2`、`2>&1`）及内存缓冲/流对象互连，无需落地临时文件；
   - 自动化脚本与 CI 流水线中应显式检查每条命令的 `exit_code`（对应 Bun `.nothrow()` 与 `.throws(bool)` 策略），避免吞掉上游管道失败状态。

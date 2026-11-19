# `cap:shell` — 嵌入式跨平台 Shell 词法/AST 引擎与进程内管道执行器

> **上游溯源**: `/data/dev/bun/src/shell/` (`shell.cp`, `interpreter.cp`, `ParsedShellScript.cp`, `Builtin.cp`, `builtin/*.cp`)  
> **模块路径**: `lib/shell/`  
> **导入协议**: `import { Shell, sh, Lexer, Token, TokenKind, Parser, Script, Statement, Pipeline, Command, ControlOp, Executor, ShellResult, isBuiltin, runBuiltin } from "cap:shell";`

---

## 一、 架构概述与设计亮点

`cap:shell` 吸收自 Bun 内置的跨平台 Shell 引擎（`Bun.$` / `bun/src/shell`），将词法分词、递归下降语法树（AST）构建、进程内跨平台 Builtin 命令直通调度与多级内存管道（In-Memory Pipeline）融为一体：

```text
lib/shell/
├── index.cp         # 模块导出门面与快捷同步执行入口 sh(...) / Shell.run(...)
├── lexer.cp         # 零堆分配 Shell 词法分词器 (Lexer, Token, TokenKind)
├── parser.cp        # 固定栈容量 AST 解析器 (Script -> Statement -> Pipeline -> Command)
├── builtin.cp       # 进程内零 fork 内置命令集 (echo, cat, pwd, cd, which, export, true, false, exit, dirname, basename)
└── exec.cp          # 管道内存流转、短路控制流 (&& / || / ;) 与文件重定向执行引擎 (Executor, ShellResult)
```

1. **零堆分配 AST 模型 (`Script` / `Statement` / `Pipeline` / `Command`)**：
   - `Script` 包含最多 `16` 条由 `;`、`&&`、`||` 连接的 `Statement`（`MAX_STATEMENTS`）。
   - 每条 `Statement` 包含一条最多 `16` 级管道命令的 `Pipeline`（`MAX_PIPELINE_CMDS`）及后继控制算子 `ControlOp` (`.None`, `.And`, `.Or`, `.Sequence`)。
   - 每个 `Command` 包含最多 `32` 个参数切片（`MAX_COMMAND_ARGS`）以及输入/输出重定向描述符（`redirect_in`, `redirect_out`, `redirect_append`）。
2. **进程内 Builtin 零子进程开销**：对于高频命令（`echo`, `cat`, `pwd`, `cd`, `which`, `dirname`, `basename`, `true`, `false`, `export`, `exit`），`Executor` 直接在当前进程内存缓冲区（`StringJoiner` & `pipe_buf`）内执行并传递管道标准输入/输出，无需调用 `fork`/`exec` 创建操作系统子进程。

---

## 二、 词法/语法支持矩阵与 `Bun.$` 对照表

### 1. 语法操作符与内蔵命令矩阵

| 语法分类 | 符号 / 命令 | AST 节点 / 语义说明 | 示例 |
| :--- | :--- | :--- | :--- |
| **管道流转** | `cmd1 \| cmd2` | `TokenKind.Pipe`：将 `cmd1` 的 `stdout` 内存缓冲直接作为 `cmd2` 的 `stdin` | `echo -n "hello" \| cat` |
| **逻辑与短路** | `cmd1 && cmd2` | `ControlOp.And`：仅当 `cmd1.exit_code == 0` 时继续执行 `cmd2` | `true && echo "ok"` |
| **逻辑或短路** | `cmd1 \|\| cmd2` | `ControlOp.Or`：仅当 `cmd1.exit_code != 0` 时回退执行 `cmd2` | `false \|\| echo "fallback"` |
| **顺序语句** | `cmd1 ; cmd2` | `ControlOp.Sequence`：无条件顺序执行多条语句 | `echo "a" ; echo "b"` |
| **覆盖/追加重定向** | `> file` / `>> file` | `RedirectOp.Out` / `Append`：将命令输出写入或追加至目标文件 | `echo "log" >> app.log` |
| **输入重定向** | `< file` | `RedirectOp.In`：指定命令输入源文件路径 | `cat < config.ini` |
| **进程内 Builtins** | `echo`, `cat`, `pwd`, `cd`, `which`, `dirname`, `basename`, `true`, `false` | 100% 纯 CP 内存态执行，跨平台行为一致 | `dirname /a/b/c.cp` |

### 2. `cap:shell` 与 `Bun.$` API 对照表

| 功能描述 | `cap:shell` API | `Bun.$` (JS/TS) 对照 | 返回值说明 |
| :--- | :--- | :--- | :--- |
| **一键执行脚本** | `sh("echo hello \| cat")` / `Shell.run(...)` | ``await $`echo hello \| cat` `` | 返回 `ShellResult` |
| **获取标准输出文本** | `res.text()` (`res.stdout_str`) | `res.text()` | `[]const u8` |
| **获取标准错误文本** | `res.err()` (`res.stderr_str`) | `res.stderr.toString()` | `[]const u8` |
| **检查退出状态** | `res.ok()` / `res.exit_code` | `res.exitCode === 0` | `bool` / `i32` |
| **独立 AST 解析** | `Parser.init(src).parseScript()` | `bun.shell.ParsedShellScript` | `Script` AST 结构 |

---

## 三、 代码示例

### 1. 快捷管道、逻辑短路与内置命令执行 (`sh` & `Shell.run`)

```typescript
const std = @import("std");
import { sh, Shell } from "cap:shell";

fn demoShellExecution(): void {
    // 1. 进程内管道流转 (echo -> cat，零子进程 fork)
    const r1 = sh("echo -n \"hello cp shell\" | cat");
    // r1.ok() => true
    // r1.text() => "hello cp shell"

    // 2. 逻辑与 (&&) / 逻辑或 (||) 短路求值
    const r2 = Shell.run("true && echo \"step_ok\"");
    // r2.text() => "step_ok\n"

    const r3 = Shell.run("false || echo \"recovered\"");
    // r3.text() => "recovered\n"

    // 3. 跨平台路径内置命令
    const r4 = sh("basename /data/dev/cp/lib/shell/index.cp");
    // r4.text() => "index.cp\n"
}
```

### 2. 词法分词器 (`Lexer`) 与语法树 (`Parser`) 检查

```typescript
import { Lexer, TokenKind, Parser, ControlOp } from "cap:shell";

fn demoAstInspection(): void {
    let parser = Parser.init("echo \"build start\" | cat > build.log && pwd");
    const script = parser.parseScript();

    // script.statements_len => 2
    // 第一条语句包含 2 个管道命令 (echo 与 cat)，且后继操作符为 ControlOp.And
    const stmt0 = script.statements[0];
    // stmt0.op == ControlOp.And
    // stmt0.pipeline.commands_len == 2
    // stmt0.pipeline.commands[1].redirect_out => "build.log"
}
```

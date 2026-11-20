# `cap:shell` — 嵌入式跨平台 Shell 词法/AST 引擎与进程内管道执行器

> **上游溯源**: `/data/dev/bun/src/shell/` (`shell.cp`, `interpreter.cp`, `ParsedShellScript.cp`, `EnvMap.cp`, `Builtin.cp`, `builtin/*.cp`) & `/data/dev/bun/src/shell_parser/`  
> **模块路径**: `lib/shell/`  
> **导入协议**: `import { Shell, sh, ShellSession, ShellEnv, DirStack, expandWord, Lexer, Token, TokenKind, QuoteKind, Parser, Script, Statement, Pipeline, Command, ControlOp, Executor, ShellResult, isBuiltin, runBuiltin, runBuiltinWithState } from "cap:shell";`

---

## 一、 架构概述与设计亮点

`cap:shell` 吸收自 Bun 内置的跨平台 Shell 引擎（`Bun.$` / `bun/src/shell`），将词法分词、递归下降 AST 解析、环境变量与引号插值展开、工作目录栈、全套进程内 POSIX Builtins 与多级管道重定向融为一体：

```text
lib/shell/
├── index.cp         # 模块导出门面与快捷同步执行入口 sh(...) / Shell.run(...) / Shell.session()
├── lexer.cp         # Shell 词法分词器 (单/双引号、注释 #、行续接、2>/2>&1/&>/>>/</| /&&/||/; /\n)
├── env.cp           # 进程内环境变量表 (ShellEnv)、工作目录栈 (DirStack) 与词展开引擎 (expandWord)
├── parser.cp        # AST 解析器 (Script -> Statement -> Pipeline -> Command + EnvAssign + Redirections)
├── builtin.cp       # 进程内零 fork 内置命令集 (23 条 POSIX/Bun Builtins)
└── exec.cp          # 会话与管道执行引擎 (ShellSession, Executor, ShellResult)
```

1. **完整词法 & 变量展开**：
   - 区分单引号字面量 `'...'`（不展开变量）、双引号 `"..."`（展开 `$VAR`、`${VAR}`、`${VAR:-default}`、`${VAR:=default}`、`${VAR:+alt}`、`${#VAR}`、`$?`、`$$` 及 `\"`/`\\`/`\$` 转义）、混合拼接词（如 `FOO="hello world"`）以及 `#` 行注释。
   - 支持赋值前缀 `FOO=bar cmd`（仅作用于单条命令，执行完自动恢复原环境）与独立赋值 `FOO=bar`（持久化到 `ShellSession`）。
2. **完整控制流与重定向**：
   - 左结合 `&&`、`||`、`;`、`\n` 链式执行与退出码短路；支持 `exit [code]` 提前终止脚本并返回退出码。
   - 支持 `<` 输入重定向、`>` / `>>` 标准输出重定向、`2>` / `2>>` 标准错误重定向、`2>&1` / `1>&2` 流合并、`&>` / `&>>` 全流重定向（基于 Zig 0.17 兼容的 `std.c.open`/`read`/`write`/`close`），以及 `ShellSession.runWithMemoryTarget` 直接重定向到内存缓冲区。
3. **23 条进程内 Builtins**：
   - `echo`（`-n`/`-e`/`-E` 及组合标志）、`printf`（`%s`/`%d`/`%i`/`%u`/`%x`/`%X`/`%c`/`%%` 与宽度对齐）、`cat`（`-n` 与 stdin/文件）、`true`、`false`、`test` / `[`（`-e`/`-f`/`-d`/`-r`/`-w`/`-x`/`-s`/`-L`/`-h`/`-z`/`-n`/`=`/`!=`/`-eq`/`-ne`/`-lt`/`-le`/`-gt`/`-ge`/`!`/`-a`/`-o`）、`pwd`、`cd`（含 `cd -`）、`pushd`、`popd`、`dirs`、`env`、`export`、`unset`、`which`（内置命令 + `$PATH` 可执行搜索）、`basename`（含可选后缀剥离）、`dirname`、`head`（`-n N` / `-N`）、`wc`（`-l`/`-w`/`-c`）、`mkdir`（`-p` 递归创建）、`rm`（`-f` / `-r` / `-R` 递归删除）、`exit`。

---

## 二、 语法特性与 Builtins 对照表

| 特性 / 命令 | 语义与支持能力 | 示例 |
| :--- | :--- | :--- |
| **引号与插值** | `'...'` 禁用插值；`"..."` 支持 `$VAR`、`${VAR:-def}`、`${VAR:=val}`、`${#VAR}`、`$?`、`$$` | `echo "hi ${USER:-guest}"` |
| **赋值前缀** | `FOO=bar cmd` 仅临时注入子命令；独立 `FOO=bar` 写入会话 | `DEBUG=1 env \| cat` |
| **逻辑短路** | `&&` / `||` / `;` / `\n` 左结合短路求值，`exit N` 中断脚本 | `false && echo no ; echo yes` |
| **重定向** | `>` / `>>` / `<` / `2>` / `2>>` / `2>&1` / `1>&2` / `&>` / `&>>` | `cat missing 2>&1 \| head -n 1` |
| **环境与目录栈** | `env` / `export` / `unset` / `pwd` / `cd` / `pushd` / `popd` / `dirs` | `pushd /tmp && pwd && popd` |
| **条件测试 `test`/`[`** | 文件属性、字符串比对、整数比较与 `!` / `-a` / `-o` 逻辑组合 | `[ -f a.txt -a 10 -gt 2 ]` |
| **文本与文件工具** | `echo -ne`, `printf`, `cat`, `head`, `wc`, `basename`, `dirname`, `mkdir -p`, `rm -rf` | `printf '%03d\n' 7 \| wc -c` |

---

## 三、 代码示例

### 1. 有状态会话 (`ShellSession`)、变量展开与目录栈

```typescript
import { Shell } from "cap:shell";

fn demoSession(): void {
    let sess = Shell.session();
    defer sess.release();

    const r1 = sess.run("export MODE=prod; echo \"mode=${MODE:-dev} len=${#MODE}\"");
    // r1.text() => "mode=prod len=4\n"

    const r2 = sess.run("mkdir -p /tmp/demo && pushd /tmp/demo > /dev/null && echo ok > out.txt && popd > /dev/null && rm -rf /tmp/demo");
    // r2.ok() => true
}
```

### 2. 内存缓冲区重定向与管道组合

```typescript
import { Shell } from "cap:shell";
import { StringJoiner } from "cap:string";

fn demoMemoryTarget(): void {
    let sess = Shell.session();
    defer sess.release();

    let out = StringJoiner.init();
    defer out.release();
    let err = StringJoiner.init();
    defer err.release();

    const code = sess.runWithMemoryTarget("printf 'a\\nb\\nc\\n' | head -n 2 | wc -l", &out, &err);
    // code == 0, out.slice() => "2\n"
}
```

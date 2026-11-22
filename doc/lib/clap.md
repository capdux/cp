# `cap:clap` — 编译期强类型与零分配流式命令行参数解析标准库

> 参考来源：`/data/dev/bun/src/clap/clap.cp`, `/data/dev/bun/src/clap/` | 吸收日期：2026-11-22
> **模块路径**: `lib/clap/`  
> **导入协议**: `import { Param, Help, ParamHelp, Values, Names, ParseOptions, parseParam, parseEx, parseArgs, usage, help, SliceIterator, ShellIterator, OsIterator, StreamingClap, ComptimeClap } from "cap:clap";`

---

## 一、 架构概述与设计亮点

`cap:clap` 吸收自 Bun 使用的高性能命令行参数解析库（`bun/src/clap`），兼具**声明式参数定义**、**零堆分配流式状态机 (`StreamingClap`)** 与**编译期强类型参数索引 (`ComptimeClap`)**：

1. **声明式参数语法解析 (`parseParam`)**：直接通过人类可读的单行帮助字符串（如 `"-p, --port <number> Server listening port"`）自动解析出短标志 `short: 'p'`、长标志 `long: "port"`、取值基数 `Values.one`、占位符名称 `"number"` 与帮助说明文本。
2. **三态流式解析状态机 (`StreamingClap`)**：
   - **`normal` 常态模式**：识别 `--long`、`--long=val`、`-s`、位置参数以及 `--` 终止符。
   - **`chaining` 短选项连写状态**：支持 POSIX 风格连写短标志（如 `-vh` 自动展开为 `-v` 与 `-h`；`-o=file.bin` 或 `-ofile.bin` 自动提取短选项参数值）。
   - **`rest_are_positional` 透传状态**：遇到独立的 `--` 后，后续所有参数（即使以 `-` 开头）均作为纯位置参数消费。
3. **编译期强类型结构化提取 (`ComptimeClap`)**：
   - 在编译期（Comptime）扫描 `params` 表，自动将布尔标志（`flags`）、单值选项（`single_options`）与多值选项（`multi_options`）分别紧凑映射为定长数组索引。
   - 当调用 `parsed.flag("--verbose")` 或 `parsed.option("--port")` 时，在编译期完成参数名校验与类型安全检查（若对布尔标志误调用 `option()`，直接在编译期触发 `@compileError`）。
4. **多源参数迭代器 (`SliceIterator` / `ShellIterator` / `OsIterator`)**：
   - `SliceIterator`：零内存分配遍历内存字符串切片 `[]const []const u8`。
   - `ShellIterator`：内置 7 状态 Shell 词法分词器，支持单引号、双引号与反斜杠转义的命令行字符串切分。
   - `OsIterator`：进程级 CLI 参数迭代器封装。

---

## 二、 参数声明语法与 `cap:clap` API 矩阵

### 1. `parseParam` 声明式语法矩阵

| 声明字符串示例 | 短标志 (`short`) | 长标志 (`long`) | 取值模式 (`takes_value`) | 语义说明 |
| :--- | :---: | :---: | :---: | :--- |
| `"-v, --verbose Enable verbose logging"` | `'v'` | `"verbose"` | `Values.none` | 布尔开关标志（不接受参数值） |
| `"-p, --port <num> Listen port"` | `'p'` | `"port"` | `Values.one` | 单值必填选项（支持 `-p 8080`, `-p=8080`, `--port=8080`） |
| `"-c, --config <file>? Optional config"` | `'c'` | `"config"` | `Values.one_optional` | 单值可选选项 |
| `"-I, --include <dir>... Search paths"` | `'I'` | `"include"` | `Values.many` | 多值可重复选项（收集所有 `-I` 值到切片） |
| `"<input> Source file to compile"` | `null` | `null` | `Values.one` | 位置参数（Positional argument） |

### 2. 核心组件与 API 对照表

| 组件 / 函数 | 用途说明 | 内存分配特性 |
| :--- | :--- | :--- |
| `parseParam(line)` | 将参数声明字符串解析为 `ParamHelp` (`Param(Help)`) | 纯计算零堆分配 |
| `SliceIterator.init(slice)` | 从 `[]const []const u8` 构建参数流迭代器 | 零堆分配 |
| `ShellIterator.init(alloc, cmd_str)` | 按 POSIX Shell 引号与转义规则切分命令行字符串 | 仅在跨引号拼接时使用内部 Arena |
| `StreamingClap(Id, Iter)` | 增量流式拉取每个解析后的 `Arg(Id) { param, value }` | 零堆分配状态机 |
| `ComptimeClap(Id, params)` | 一次性解析并按标志名强类型查询 `flag()`, `option()`, `options()`, `positionals()` | 仅对多值选项与位置参数列表分配结果切片 |
| `usage(stream, params)` | 自动生成紧凑的命令行用法摘要（如 `[-vh] [-p <num>] <input>`） | 流式写入零分配 |
| `help(stream, params)` | 自动对齐列宽并输出完整的命令行帮助文档列表 | 流式写入零分配 |

---

## 三、 代码示例

### 1. `ComptimeClap` 编译期强类型参数解析

```typescript
const std = @import("std");
import {
    Help,
    ParamHelp,
    ParseOptions,
    parseParam,
    SliceIterator,
    ComptimeClap,
} from "cap:clap";

const p_help = (parseParam("-h, --help Display this help message") catch unreachable);
const p_verb = (parseParam("-v, --verbose Enable verbose output") catch unreachable);
const p_port = (parseParam("-p, --port <number> Server listening port") catch unreachable);
const p_inc  = (parseParam("-I, --include <dir>... Add include directory") catch unreachable);
const p_file = (parseParam("<entry> Main entry file") catch unreachable);
const cli_params: [5]ParamHelp = [p_help, p_verb, p_port, p_inc, p_file];

fn demoComptimeClap(): !void {
    const argv: [7][]const u8 = [
        "-v",
        "--port=8080",
        "-I", "/usr/include",
        "--include=/opt/local/include",
        "src/main.cp",
    ];

    let iter = SliceIterator.init(&argv);
    const ClapEngine = ComptimeClap(Help, &cli_params);
    let args = try ClapEngine.parse(&iter, ParseOptions{
        allocator: std.heap.page_allocator,
    });
    defer args.deinit();

    // 1. 读取布尔标志（支持短名或长名查询）
    let is_verbose = args.flag("-v");          // true
    let is_help = args.flag("--help");         // false

    // 2. 读取单值选项
    let port_str = args.option("--port").?;    // "8080"

    // 3. 读取多值可重复选项
    let includes = args.options("--include");  // ["/usr/include", "/opt/local/include"]

    // 4. 读取位置参数
    let positionals = args.positionals();      // ["src/main.cp"]
}
```

### 2. `StreamingClap` 零分配流式参数解析状态机

```typescript
import {
    Help,
    ParamHelp,
    parseParam,
    SliceIterator,
    StreamingClap,
} from "cap:clap";

fn demoStreamingClap(): !void {
    const p1 = (parseParam("-v, --verbose Verbose mode") catch unreachable);
    const p2 = (parseParam("-o, --out <file> Output path") catch unreachable);
    const p3 = (parseParam("<target> Input target") catch unreachable);
    const params: []const ParamHelp = [p1, p2, p3];

    const raw_args: [4][]const u8 = ["-v", "-o", "app.bin", "main.cp"];
    let iter = SliceIterator.init(&raw_args);

    const StreamParser = StreamingClap(Help, SliceIterator);
    let stream = StreamParser{
        params: &params,
        iter: &iter,
    };

    while (arg := try stream.next()) {
        // 逐个流式处理解析出的参数项 arg.param 与 arg.value
    }
}
```

### 3. `ShellIterator` 命令行字符串词法切分

```typescript
const std = @import("std");
import { ShellIterator } from "cap:clap";

fn demoShellTokenizer(): !void {
    const cmd = "cap build \"src/my app.cp\" --define='MODE=prod' --out=bin/app\\ 1";
    let iter = ShellIterator.init(std.heap.page_allocator, cmd);
    defer iter.deinit();

    while (tok := try iter.next()) {
        // 依次产出:
        // "cap", "build", "src/my app.cp", "--define=MODE=prod", "--out=bin/app 1"
    }
}
```

---

## 上游 Bun `clap` 行为契约与边界注意事项

1. **短标志连写与取值截断（`StreamingClap`）**：
   - 在 `chaining` 状态下，布尔短标志可连续组合（如 `-vf` 展开为 `-v` 与 `-f`）；一旦遇到需要取值的短选项（如 `-ofile` 或 `-o=file`），该选项之后的剩余字符立即作为其参数值消费并退出连写状态。
2. **`--` 透传哨兵与负数值区分**：
   - 遇到独立 `--` 后进入 `rest_are_positional` 状态，后续所有以 `-` 开头的参数（包括负数或子命令标志）均原样作为位置参数返回。
3. **编译期参数名零拼写错误保障（`ComptimeClap`）**：
   - 通过 `ComptimeClap` 声明参数表时，对不存在的标志名或类型不匹配（如对布尔标志调用 `option`）会在编译期直接报错拦截，消除运行时静默忽略拼错参数的隐患。

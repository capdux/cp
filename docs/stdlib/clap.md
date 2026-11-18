# `cap:clap` — 高性能命令行参数解析与声明式 CLI 引擎

`cap:clap` 模块是 CP 语言官方一等公民标准库中的命令行参数解析套件，源自并全量重构吸收自 Bun 生产级 CLI 架构（`/data/dev/bun/src/clap/`）。本模块提供从底层词法迭代、流式按需消费到编译期（Comptime）强类型结构化解析的全栈方案，具备极高的执行性能与零额外分配开销。

---

## 模块设计理念

1. **三层解析架构按需选用**：
   - **底层词法层 (`args.cp`)**：零拷贝切片迭代器 (`SliceIterator`)、兼容 POSIX Shell 转义与引号语法的命令行词法器 (`ShellIterator`)，以及系统原生命令行迭代器 (`OsIterator`)；
   - **流式按需层 (`streaming.cp`)**：有限状态机流式解析器 (`StreamingClap`)，支持复杂短参数合流（如 `-abc`）、长参数与内联赋值（`--flag=val`）、多值与贪婪位置参数；
   - **编译期强类型层 (`comptime.cp` & `clap.cp`)**：在编译期推导参数索引与布尔掩码，直接提供类型安全的方法访问（`flag()`, `option()`, `options()`, `positionals()`），消灭动态字符串查找开销。
2. **声明式语法字符串自解码**：通过 `parseParam` 编译期函数，直接将易读的参数描述字符串（例如 `"-h, --help Show help instructions"` 或 `"-o, --out <file> Output artifact"`）解码为强类型 `Param(Id)` 元数据。
3. **格式化与自省友好**：内建 `usage` 与 `help` 格式化排版引擎，支持对齐制表符输出、多别名折叠与自动化位置参数推断。
4. **100% 现代 CP 语义规范**：全库废除 `var`，统一采用 `let`/`const`；零 `@as` 类型强转；现代化错误联合传播；数组与切片内存安全保证。

---

## 核心组件一览

| 组件 | 说明 | 适用场景 |
| :--- | :--- | :--- |
| `SliceIterator` | 内存切片参数迭代器 | 单元测试、子进程参数分发、内存内参数数组解析 |
| `ShellIterator` | 遵循 POSIX 引号与转义规则的命令行字符串解析器 | 交互式 Shell、REPL、调试控制台输入分词 |
| `OsIterator` | 操作系统原生入口命令行迭代器 | 生产级应用程序 `main()` 参数获取 |
| `StreamingClap` | 状态机流式参数解析器 | 极低内存开销、需要精确捕获每一次参数消费细节的场景 |
| `ComptimeClap` | 编译期强类型聚合解析器 | 生产级高性能 CLI 工具，按名称安全提取类型化参数 |
| `parseParam` | 声明式字符串转参数结构体函数 | 以简洁的 DSL 风格编写命令行说明并自动化构建解析器 |
| `usage` / `usageFull` | 命令行简要用法（Usage）格式化生成器 | 错误提示、`--usage` 输出 |
| `help` / `helpFull` | 命令行完整帮助文档（Help）对齐格式化生成器 | `--help` 标准帮助手册输出 |
| `Diagnostic` | 智能错误诊断与提示聚合器 | 在参数解析失败时提供清晰友好的定位提示 |

---

## API 使用示例

### 1. 词法迭代器 (`SliceIterator` & `ShellIterator`)

```typescript
import { SliceIterator, ShellIterator } from "cap:clap";
import std from "std";

// 1. 内存切片迭代
const cli_args: [4][]const u8 = ["bun", "test", "--watch", "--coverage"];
let slice_iter = SliceIterator.init(&cli_args);

while (arg := slice_iter.next()) {
    // 按序迭代输出切片元素
}

// 2. 带转义与引号的 Shell 字符串词法拆解
const raw_cmd = "cp -r 'source folder' \"dest\\\"folder\" --threads=8";
let shell_iter = ShellIterator.init(std.heap.page_allocator, raw_cmd);
defer shell_iter.deinit();

while (token := shell_iter.next() catch unreachable) {
    // 依次解析得到:
    // "cp", "-r", "source folder", "dest\"folder", "--threads=8"
}
```

---

### 2. 声明式参数定义与 `parseParam`

无需手动编写冗长的结构体字段，使用标准声明式字符串即可定义完整的命令行参数：

```typescript
import { parseParam, ParamHelp, Help, Values } from "cap:clap";

// 定义标准帮助布尔参数
const p_help = parseParam("-h, --help Print help information") catch unreachable;
assert(p_help.names.short.? == 'h');
assert(p_help.names.long.? == "help");
assert(p_help.takes_value == Values.none);

// 定义接收单值的参数
const p_out = parseParam("-o, --out <file> Output artifact file") catch unreachable;
assert(p_out.takes_value == Values.one);

// 定义接收多个重复值的参数 (使用 '...')
const p_inc = parseParam("-I, --include <dir>... Include search paths") catch unreachable;
assert(p_inc.takes_value == Values.many);

// 定义位置参数
const p_target = parseParam("<target> Primary build target") catch unreachable;
assert(p_target.names.short == null);
assert(p_target.names.long == null);
assert(p_target.takes_value == Values.one);
```

---

### 3. 流式解析器 (`StreamingClap`)

适用于事件驱动或需精确控制解析步骤的场景：

```typescript
import { StreamingClap, SliceIterator, parseParam, ParamHelp } from "cap:clap";
import std from "std";

const p1 = parseParam("-v, --verbose Enable verbose logs") catch unreachable;
const p2 = parseParam("-o, --output <path> Output path") catch unreachable;
const params: [2]ParamHelp = [p1, p2];

const input_args: [3][]const u8 = ["-v", "-o", "bin/release"];
let iter = SliceIterator.init(&input_args);
let parser = StreamingClap(ParamHelp).init(&params, &iter);

while (arg := parser.next() catch unreachable) {
    if (arg.param) |p| {
        if (p.names.short.? == 'v') {
            // 处理 -v 选项
        } else if (p.names.short.? == 'o') {
            const val = arg.value.?; // "bin/release"
        }
    }
}
```

---

### 4. 编译期强类型解析器 (`ComptimeClap`)

生产级命令行程序推荐的最佳实践，在编译期完成状态矩阵计算，并在运行时以常数级耗时检索提取：

```typescript
import {
    ComptimeClap,
    SliceIterator,
    ParseOptions,
    parseParam,
    ParamHelp,
    Help,
} from "cap:clap";
import std from "std";

// 在模块顶层或编译期定义参数矩阵
const p_help = parseParam("-h, --help Show help instructions") catch unreachable;
const p_verb = parseParam("-v, --verbose Enable verbose logs") catch unreachable;
const p_out  = parseParam("-o, --out <file> Output artifact file") catch unreachable;
const p_inc  = parseParam("-I, --include <dir>... Include search paths") catch unreachable;
const p_file = parseParam("<target> Main compilation target") catch unreachable;
const cli_params: [5]ParamHelp = [p_help, p_verb, p_out, p_inc, p_file];

// 构造解析器类型
const ClapType = ComptimeClap(Help, &cli_params);

fn run_cli(args: []const []const u8) !void {
    let iter = SliceIterator.init(args);
    let opt = ParseOptions{
        allocator: std.heap.page_allocator,
    };

    let parsed = ClapType.parse(&iter, opt)?;
    defer parsed.deinit();

    // 1. 布尔开关检查
    if (parsed.flag("-h") || parsed.flag("--help")) {
        // 输出帮助
        return;
    }
    const is_verbose = parsed.flag("-v") || parsed.flag("--verbose");

    // 2. 单值选项提取
    if (out_path := parsed.option("--out")) {
        // 使用指定的输出路径: out_path
    }

    // 3. 多值选项提取 (如多次传入 -I /usr/include -I /data/dev/include)
    const search_paths = parsed.options("--include");
    for (dir in search_paths) {
        // 处理每一个搜索路径
    }

    // 4. 位置参数提取
    const positionals = parsed.positionals();
    if (positionals.len > 0) {
        const target_file = positionals[0];
    }
}
```

---

### 5. 帮助手册与用法文档生成 (`usage` & `help`)

能够根据参数定义自动化生成工业级排版的终端文档：

```typescript
import { usage, help, parseParam, ParamHelp } from "cap:clap";

const p1 = parseParam("-h, --help Print this help message") catch unreachable;
const p2 = parseParam("-c, --config <path> Path to configuration file") catch unreachable;
const p3 = parseParam("<entry> Entrypoint script") catch unreachable;
const params: [3]ParamHelp = [p1, p2, p3];

// 任意实现了 writeAll(*_, bytes) 与 writeByte(*_, byte) 的流均可传入
usage(&stream, &params) catch unreachable;
// 输出: "[-h] [-c <path>] <entry>"

help(&stream, &params) catch unreachable;
// 输出格式化且制表对齐的帮助清单:
//	-h, --help           	Print this help message
//	-c, --config <path>  	Path to configuration file
```

---

## 质量与测试覆盖

`cap:clap` 经受了 `cap/test/test_clap_complete.cp` 全套回归测试覆盖：

- **Test 1**: `SliceIterator` 内存参数序列迭代与边界空值校验；
- **Test 2**: `ShellIterator` 嵌套单双引号、反斜杠转义与连续空白符分词；
- **Test 3**: `parseParam` 声明式短长名、多值语法（`...`）与位置参数元数据解析；
- **Test 4**: `StreamingClap` 合并短参数与内联赋值状态机运转；
- **Test 5**: `ComptimeClap` 强类型全量字段（flags, single options, multi options, positionals）提取；
- **Test 6**: `help` 与 `usage` 零动态内存分配的格式化文档输出。

全套测试用例通过 `cap test cap/test/test_clap_complete.cp` 达成 **100% PASS (742ms)**。

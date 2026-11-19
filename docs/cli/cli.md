# CP 标准库：零分配命令行参数解析、终端配色、进度条与表格渲染手册 (`cap:cli`)

## 一、 架构总览与设计哲学

`cap:cli` 是 CP 语言内置的**命令行工具开发套件**，吸收自 Bun 的 `src/clap/args.cp`、`src/bun_core/tty.cp` 与 `src/cli/`。全套模块坚持**零堆分配（Zero-Allocation）**设计：参数切片直接引用 `argv` 原内存，ANSI 剥离、进度条与 ASCII 表格直接渲染至调用方栈缓冲区，启动延迟低至微秒级。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                           cap:cli 工具链分层                                │
│                                                                            │
│  ┌──────────────────────────────┐   ┌───────────────────────────────────┐  │
│  │ 参数解析层 (args.cp)          │   │ 终端控制与渲染层 (tty.cp/table.cp) │  │
│  │  • SliceIterator (参数流)     │   │  • TtyMode (Normal / Raw / Io)    │  │
│  │  • CliArgs.parse(argv)       │   │  • AnsiColor (CSI 常量 & stripAnsi)│  │
│  │    --flag / --k=v / -abc / --│   │  • ProgressBar ([#####-----])     │  │
│  │  • hasFlag / getOption       │   │  • CliTable (自动列宽/左中右对齐)   │  │
│  └──────────────────────────────┘   └───────────────────────────────────┘  │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心 API 详解与代码示例

### 1. 命令行参数解析 (`CliArgs` & `SliceIterator`)

支持长标志（`--release`）、长选项（`--target=x86_64-linux`）、组合短标志（`-vf` 等价 `-v -f`）、短选项赋值（`-p=8080`）以及 `--` 双横线之后的所有原始位置参数：

```typescript
import { CliArgs, SliceIterator } from "cap:cli";

pub fn demoArgs() {
    let argv: [6][]const u8 = undefined;
    argv[0] = "build";
    argv[1] = "--release";
    argv[2] = "--out=dist";
    argv[3] = "-vf";
    argv[4] = "--";
    argv[5] = "--literal-file";

    const args = CliArgs.parse(&argv);
    const is_release = args.hasFlag("release");                // true
    const verbose = args.hasFlag("v");                         // true
    const out_dir = args.getOptionOr("out", "build");          // "dist"
    const cmd = args.positional(0);                            // "build"
    const literal = args.positional(1);                        // "--literal-file"
}
```

### 2. 终端模式与 ANSI 配色 (`TtyMode` & `AnsiColor`)

提供标准 ANSI SGR 常量（`RESET`/`BOLD`/`DIM`/`RED`/`GREEN`/`YELLOW`/`BLUE`/`MAGENTA`/`CYAN`/`GRAY`）及 `AnsiColor.stripAnsi` 用于日志落盘或计算可见字符宽度：

```typescript
import { TtyMode, AnsiColor } from "cap:cli";

pub fn demoAnsi() {
    const mode = TtyMode.Raw;
    let out: [128]u8 = undefined;
    const clean = AnsiColor.stripAnsi("\x1b[32mOK\x1b[0m built in 12ms", &out);
    // clean == "OK built in 12ms"
}
```

### 3. 确定性 ASCII 进度条 (`ProgressBar` / `Progress`)

```typescript
import { ProgressBar } from "cap:cli";

pub fn demoProgress() {
    let pb = ProgressBar.init(100, 20);
    pb.tick(35);
    const pct = pb.percent(); // 35

    let buf: [32]u8 = undefined;
    const bar_str = pb.renderBar(&buf); // "[#######-------------]"
    pb.set(100);
    const done = pb.isComplete();       // true
}
```

### 4. 自动列宽 ASCII 表格 (`CliTable` & `CliTableAlign`)

支持最多 8 列、32 行零堆分配自动计算列宽，并支持 `Left`、`Center`、`Right` 三种单元格对齐模式：

```typescript
import { CliTable, CliTableAlign } from "cap:cli";

pub fn demoTable() {
    let tbl = CliTable.init(3);
    tbl.setHeader(0, "Module", CliTableAlign.Left);
    tbl.setHeader(1, "Status", CliTableAlign.Center);
    tbl.setHeader(2, "Latency", CliTableAlign.Right);

    * = tbl.addRow3("cap:http", "PASS", "12ms");
    * = tbl.addRow3("cap:bench", "PASS", "2ms");

    let out: [1024]u8 = undefined;
    const len = tbl.renderToBuf(&out);
    const table_text = out[0..len];
}
```

---

## 三、 与 Node.js / Bun CLI 能力对照表

| 能力 | Node.js / Bun | CP (`cap:cli`) | 核心优势 |
|---|---|---|---|
| 参数解析 | `util.parseArgs()` / `commander` | `CliArgs.parse(argv)` | 64 选项 + 64 位置参数栈内解析，零堆分配 |
| ANSI 剥离 | `Bun.stripANSI(str)` / `util.stripVTControlCharacters` | `AnsiColor.stripAnsi(input, &buf)` | 零拷贝写入栈缓冲，无临时字符串分配 |
| 终端模式 | `process.stdin.setRawMode(true)` | `TtyMode` (`Normal`/`Raw`/`Io`) | 强类型 C-ABI 枚举直通底层 TTY 驱动 |
| 进度条与表格 | `cli-progress` / `console.table` | `ProgressBar` & `CliTable` | 零依赖内置标准库，确定性输出到字节切片 |

# CP 标准库：结构化编译器与运行时诊断日志引擎手册 (`cap:logger`)

## 一、 架构总览与设计准则

`cap:logger` 是 CP 语言官方内置的结构化源码诊断与日志记录标准库，吸收自 Bun 的编译器/打包器核心日志引擎（`/data/dev/bun/src/logger/logger.cp`），以 **4 字节轻量源码偏移 `Loc` + 行列反查 `Source` + 零堆分配诊断累加器 `Log` + 纯文本/ANSI 富终端渲染** 为核心，广泛适用于编译器、Linter、配置解析器与高并发服务端诊断报告。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                       CP 用户层 / 编译器前端 / 工具链                   │
├────────────────────────────────────────────────────────────────────────┤
│  Log (定容 64 条零堆累加器: level 过滤 · errors/warnings 计数 · formatAll)│
├───────────────────────────────────┬────────────────────────────────────┤
│     DiagMsg (结构化诊断消息)       │       Source (源码视图与定位)       │
│  level · text · file · line:col   │  path · contents                   │
│  line_text · range_len · note     │  lineColOf(offset) · lineTextOf()  │
│  formatPlain() · formatAnsi()     │  Loc (i32) · Range (loc + len)     │
└───────────────────────────────────┴────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/logger/index.cp` | `// Source: /data/dev/bun/src/logger/logger.cp` | `cap:logger` 统一公开导出入口 |
| `lib/logger/logger.cp` | `// Source: /data/dev/bun/src/logger/logger.cp` | `LogLevel`、`Loc`、`Range`、`LineCol`、`Source`、`DiagMsg` 与 `Log` |

---

## 二、 核心 API 详解

### 1. 日志级别 (`LogLevel`)

枚举值：`Debug = 0`、`Info = 1`、`Warn = 2`、`Err = 3`、`Verbose = 4`。
- `level.label(): []const u8` -> `"debug"` / `"info"` / `"warn"` / `"error"` / `"verbose"`。
- `level.ansiColor(): []const u8` -> 对应 ANSI 终端颜色控制前缀。
- `level.shouldEmit(min_level: LogLevel): bool` -> 判断当前级别消息是否应被记录输出。

### 2. 源码定位与切片 (`Loc`, `Range`, `LineCol`, `Source`)

| API 签名 | 说明 |
| :--- | :--- |
| `Loc.init(start: i32)` / `Loc.empty()` / `loc.isEmpty()` / `loc.toUsize()` | 4 字节轻量字节偏移（`-1` 表示无位置） |
| `Range.init(start, len)` / `range.end()` / `range.sliceOf(contents)` | 源码字节区间与安全子切片提取 |
| `Source.init(path, contents)` | 构造不可变源码文件视图 |
| `src.lineColOf(offset: usize): LineCol` | 计算字节偏移对应的 1-based 行号、列号及所在行起止字节偏移 |
| `src.lineTextOf(offset: usize): []const u8` | 零拷贝返回该偏移所在的完整代码行（自动去除末尾 `\r\n`） |

### 3. 诊断消息与累加器 (`DiagMsg`, `Log`)

| API 签名 | 说明 |
| :--- | :--- |
| `DiagMsg.fromSource(level, src, range, text, note): DiagMsg` | 自动通过 `Source.lineColOf` 填充行列号、代码行摘录与注释提示 |
| `msg.formatPlain(out_buf)` / `msg.formatAnsi(out_buf)` | 输出带 `--> file:line:col`、源码行、`^~~~` 范围下划线与 `= note:` 的格式化文本 |
| `Log.init(min_level): Log` | 初始化定容（`LOG_MAX_MSGS = 64`）零堆分配诊断累加器 |
| `log.addError(src?, loc, text)` / `log.addErrorRange(src, range, text, note)` | 递增 `errors` 计数并按级别记录错误诊断 |
| `log.addWarning(src?, loc, text)` / `log.addWarningRange(src, range, text, note)` | 递增 `warnings` 计数并记录警告诊断 |
| `log.addInfo(text)` / `log.addDebug(text)` / `log.addMsg(msg)` | 记录通用日志消息 |
| `log.hasErrors()` / `log.clear()` / `log.formatAll(out_buf, use_ansi)` | 检查是否存在错误、重置或一次性汇总格式化全部消息 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { LogLevel, Loc, Range, Source, DiagMsg, Log } from "cap:logger";

pub fn demoLogger() {
    const code = "let count = 1;\nlet total = missing_var + 2;\n";
    const src = Source.init("src/calc.cp", code);

    let log = Log.init(LogLevel.Warn);
    log.addDebug("ignored under Warn level");
    log.addWarning(src, Loc.init(4), "unused variable 'count'");

    const off: usize = std.mem.indexOf(u8, code, "missing_var").?;
    log.addErrorRange(
        src,
        Range.init(off as i32, 11),
        "undefined identifier 'missing_var'",
        "did you mean 'count'?",
    );

    if (log.hasErrors()) {
        let report_buf: [1024]u8 = [0; 1024];
        const plain = log.formatAll(report_buf[0..1024], false);
        // 输出包含:
        // warn: unused variable 'count'
        //   --> src/calc.cp:1:5
        // error: undefined identifier 'missing_var'
        //   --> src/calc.cp:2:13
        //    | let total = missing_var + 2;
        //    |             ^~~~~~~~~~~
        //    = note: did you mean 'count'?
        _ = plain;
    }
}
```

---

## 四、 与 Bun `src/logger/logger.cp` 对照表

| 功能分类 | Bun (`src/logger/logger.cp`) | CP 标准库 (`cap:logger`) |
| :--- | :--- | :--- |
| **日志与诊断级别** | `logger.Log.Level` / `logger.Msg.Kind` | `LogLevel` (`Debug`, `Info`, `Warn`, `Err`, `Verbose`) |
| **4 字节源码偏移** | `logger.Loc` (`start: i32 = -1`) | `Loc` (`start: i32 = -1`) |
| **源码区间** | `logger.Range` (`loc`, `len`) | `Range` (`loc`, `len`, `sliceOf`) |
| **源码文件与行列定位** | `logger.Source` / `computeLineAndColumn` | `Source` / `lineColOf(offset)` / `lineTextOf(offset)` |
| **结构化诊断消息** | `logger.Msg` + `Notes` | `DiagMsg` + `formatPlain` / `formatAnsi` |
| **错误/警告累加器** | `logger.Log` (`errors`, `warnings`, `msgs`) | `Log` (`errors`, `warnings`, `msgs`, `formatAll`) |

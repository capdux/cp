# CP 标准库：结构化编译器与运行时诊断日志引擎手册 (`cap:logger`)

> **上游溯源**: `/data/dev/bun/src/logger/logger.cp`  
> **模块路径**: `lib/logger/`  
> **导入协议**: `import { LogLevel, Loc, Range, LineCol, LineIndex, Source, DiagNote, DiagMsg, Log } from "cap:logger";`

---

## 一、 架构总览与核心能力

`cap:logger` 吸收自 Bun 编译器/打包器的核心诊断引擎，采用 **4 字节轻量源码偏移 `Loc` + O(log N) 行索引缓存 `LineIndex` + 多重子注释 `DiagNote` + `max_errors` 熔断器 + Plain / ANSI / JSON 多格式输出** 设计：

```text
┌────────────────────────────────────────────────────────────────────────────┐
│  Log (Level 过滤 · max_errors 熔断器 · merge 多任务合并 · formatAll/Json)  │
├──────────────────────────────────────┬─────────────────────────────────────┤
│  DiagMsg (主诊断消息)                 │  Source + LineIndex (源码视图)       │
│  • 1 行前置上下文窗口 (prev_line_text)│  • LineIndex.build() 预扫描行偏移   │
│  • ^~~~ 区间下划线标尺                │  • O(log N) 二分查找 lineColOf()    │
│  • notes[8]: DiagNote 独立源码定位    │  • lineTextByNumber(line)           │
│  • formatPlain / formatAnsi / Json   │  • Loc (i32) / Range (loc + len)    │
└──────────────────────────────────────┴─────────────────────────────────────┘
```

### 核心特性一览

1. **完整日志级别与 `max_errors` 熔断器**：
   - `LogLevel` (`Verbose`, `Debug`, `Info`, `Warn`, `Err`) 支持 `shouldEmit(min_level)`、`atLeast(other)` 与 `LogLevel.fromString(...)`。
   - `Log.initWithMaxErrors(level, max_errors)` / `log.setMaxErrors(max)` 在错误达到阈值后自动触发熔断（`isErrorLimitReached()`），后续错误计入 `suppressed_errors` 避免日志雪崩。
2. **附加子注释 (`DiagNote`) 与跨位置引用**：
   - 每条 `DiagMsg` 除兼容单字符串 `note` 外，还支持挂载最多 8 条 `DiagNote`（每条可携带独立的 `Source`/`Range`，渲染为 `= note: ...` + `--> file:line:col` + 源码行与 `^~~~` 标尺）。
3. **高性能 `LineIndex` 行缓存与上下文窗口**：
   - `Source.initIndexed(path, contents)` 或 `src.buildLineIndex()` 预先构建行偏移索引，将 `lineColOf(offset)` 从 $O(N)$ 提升为 $O(\log L)$ 二分查找。
   - `DiagMsg.fromSource` 自动提取前 1 行上下文（`prev_line_text`）并输出带行号对齐的双行源码窗口与 `^~~~` 区间下划线。
4. **格式化辅助、子任务合并与 JSON 输出**：
   - `addErrorFmt` / `addErrorRangeFmt` / `addWarningFmt` 原地格式化错误消息。
   - `log.merge(&other_log)` 汇总并行子任务收集的诊断信息（自动同步错误阈值与过滤规则）。
   - `log.formatJson(out_buf)` / `msg.formatJson(out_buf)` 输出带转义的机器可读 JSON 诊断数组。

---

## 二、 可运行代码示例

```typescript
const std = @import("std");
import { LogLevel, Loc, Range, Source, DiagNote, DiagMsg, Log } from "cap:logger";

pub fn demoRichDiagnostics() {
    const code = "let count = 1;\nlet count = 2;\nreturn count;\n";
    const src = Source.initIndexed("src/calc.cp", code);

    let msg = DiagMsg.fromSource(
        LogLevel.Err,
        src,
        Range.init(19, 5),
        "redeclaration of 'count'",
        "",
    );
    msg.code = "E0201";
    * = msg.addNoteFromSource(src, Range.init(4, 5), "previous declaration here");

    let log = Log.initWithMaxErrors(LogLevel.Info, 10);
    log.addMsg(msg);

    let fmt_buf: [128]u8 = [0; 128];
    log.addErrorFmt(src, Loc.init(30), fmt_buf[0..128], "invalid return code {d}", .{1});

    let plain_buf: [2048]u8 = [0; 2048];
    const plain = log.formatAll(plain_buf[0..2048], false);

    let json_buf: [2048]u8 = [0; 2048];
    const json_arr = log.formatJson(json_buf[0..2048]);
    * = plain;
    * = json_arr;
}
```

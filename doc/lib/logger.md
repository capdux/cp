# CP 标准库：结构化日志与分布式可观测性引擎手册 (`cap:logger`)

> **上游溯源**: `/data/dev/bun/src/logger/logger.cp` + CP 生产级可观测性扩展  
> **模块路径**: `lib/logger/`（`logger.cp`, `structured.cp`, `trace.cp`, `sampler.cp`, `index.cp`）  
> **导入协议**: `import { LogLevel, Log, StructuredLogger, LogFormat, LogField, TraceParent, TraceState, Span, SpanStatus, RatioSampler, BurstSampler, RingLogBuffer } from "cap:logger";`

---

## 一、 架构总览与核心能力

`cap:logger` 提供全零堆分配的工业级双引擎能力：**编译器/源码级诊断引擎**（源自 Bun `src/logger`）与 **生产级结构化日志 & W3C Trace Context / OpenTelemetry 追踪采样引擎**：

```text
┌────────────────────────────────────────────────────────────────────────────┐
│  1. StructuredLogger & LogFormatter (lib/logger/structured.cp)             │
│     • LogFormat: Json / Logfmt / PrettyAnsi                                │
│     • 严格 RFC 8259 JSON 转义 (\" \\ \n \r \t \u00XX)                      │
│     • withField/withStr/withTrace 子上下文绑定 & 回调/内存缓冲 Sink           │
│     • redactKeys 自动脱敏 (password/token/authorization/secret -> REDACTED) │
├────────────────────────────────────────────────────────────────────────────┤
│  2. W3C Trace Context & OTel Span Tracer (lib/logger/trace.cp)             │
│     • TraceParent: 严格 00-<32hex>-<16hex>-<2hex> 解析/全零拒绝/childSpan   │
│     • TraceState: W3C tracestate 解析、查询、置顶更新与格式化               │
│     • Span + SpanEvent + SpanStatus: 起止纳秒时间戳、属性/事件与 OTel JSON   │
├────────────────────────────────────────────────────────────────────────────┤
│  3. Deterministic Samplers & Ring Buffer (lib/logger/sampler.cp)           │
│     • RatioSampler: 基于 TraceId/Key 确定性哈希的 0.0..1.0 比例采样         │
│     • BurstSampler: 窗口内前 N 条全量放行、超出后 1/M 降采样防日志风暴       │
│     • RingLogBuffer<CAP>: 固定容量零泄漏环形日志缓冲与崩溃回放导出           │
├────────────────────────────────────────────────────────────────────────────┤
│  4. 源码级诊断日志 (lib/logger/logger.cp)                                   │
│     • LogLevel / Loc / Range / Source / LineIndex O(log N) / DiagNote      │
│     • DiagMsg (上下文窗口 + ^~~~ 标尺) / Log (max_errors 熔断器 + merge)     │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心模块详解

### 1. 结构化日志引擎 (`lib/logger/structured.cp`)

- **`LogFieldValue` / `LogField`**：强类型带标签联合字段（`Str`, `Int`, `Uint`, `Float`, `Bool`, `Null`）。
- **`LogFormat`**：支持 `Json`、`Logfmt`、`PrettyAnsi` 三种输出格式。
- **`LogFormatter`**：
  - `escapeJsonString`：严格转义 `\"`、`\\`、`\n`、`\r`、`\t` 以及 `< 0x20` 控制字符为 `\u00XX`。
  - `formatJson` / `formatLogfmt` / `formatPrettyAnsi` / `format`。
  - 内置不区分大小写脱敏键：`password`、`token`、`authorization`、`secret`（命中自动输出 `"[REDACTED]"`），并支持通过 `addRedactKey` 追加自定义键。
- **`StructuredLogger`**：
  - `init(min_level, format)` / `initNamed(name, min_level, format)`。
  - `withField` / `withStr` / `withInt` / `withUint` / `withBool` / `withTrace` 派生子 Logger。
  - 支持固定内存缓冲（`captured()` / `clearBuffer()`）以及自定义回调 `setSink(cb, ctx)`。

### 2. W3C Trace Context 与轻量级 Span 追踪 (`lib/logger/trace.cp`)

- **`TraceParent`**（W3C Trace Context Level 1）：
  - `TraceParent.parse(header)`：严格验证 55 字节格式 `00-<32hex>-<16hex>-<2hex>`，校验版本号为 `00`、全小写十六进制字符，并拒绝全零 `trace_id` 或全零 `span_id`。
  - `isSampled()` / `setSampled(bool)` / `format(out_buf)` / `childSpan(new_span_id)`。
- **`TraceState`**：
  - `TraceState.parse(header)`：遵循 W3C `tracestate` 键值与多租户 `tenant@vendor` 语法校验。
  - `get(key)`、`set(key, value)`（按 W3C 规范更新或插入时自动置顶至列表最左侧）、`remove(key)`、`format(out_buf)`。
- **`SpanStatus` / `SpanEvent` / `Span`**：
  - `Span.start` / `Span.startAt` / `Span.startChild` / `Span.startChildAt`。
  - `setServiceName`、`setAttr*`、`addEvent` / `addEventAt`、`setStatus(SpanStatus, msg)`、`end` / `endAt`、`durationNs()`。
  - `toJson(out_buf)`：导出 OpenTelemetry 风格 JSON（含 `traceId`、`spanId`、`parentSpanId`、`startTimeUnixNano`、`endTimeUnixNano`、`durationNs`、`status`、`attributes`、`events`）。

### 3. 采样器与环形缓冲 (`lib/logger/sampler.cp`)

- **`RatioSampler`**：`init(ratio)` / `initWithSeed(ratio, seed)`，基于 FNV-1a + SplitMix64 对 `TraceId` 或任意 `key` 进行确定性哈希采样（`shouldSample(key)` / `shouldSampleTrace(tp)`）。
- **`BurstSampler`**：`init(burst_limit, downsample_rate, window_ns)`，单窗口内前 `N` 条全量放行，超出后按 `1/M` 降采样，跨窗口自动重置计数（`shouldSample()` / `shouldSampleAt(now_ns)`）。
- **`RingLogBuffer<const CAP: usize>`**：零堆分配固定容量环形缓冲，`push` 自动覆盖最旧记录并统计 `overwrittenCount()`，支持 `get(idx)`、`latest()`、`dumpJson` 与 `dumpLogfmt`。

### 4. 源码级诊断日志 (`lib/logger/logger.cp`)

- **`LogLevel`** (`Verbose`, `Debug`, `Info`, `Warn`, `Err`)：`shouldEmit(min_level)`、`atLeast(other)`、`LogLevel.fromString(...)`。
- **`DiagNote` & `DiagMsg`**：支持最多 8 条跨源码位置子注释，带 1 行前置上下文窗口与 `^~~~` 区间下划线。
- **`LineIndex` & `Source`**：`LineIndex.build()` 预扫描行偏移，提供 $O(\log L)$ 二分查找 `lineColOf()`。
- **`Log`**：支持 `max_errors` 熔断器、`merge(&other_log)`、Plain / ANSI / JSON 输出。

---

## 三、 可运行代码示例

### 1. 结构化日志、脱敏与 W3C Trace 追踪

```typescript
import {
    LogLevel,
    LogFormat,
    LogField,
    StructuredLogger,
    TraceParent,
    Span,
    SpanStatus,
    RatioSampler,
    BurstSampler,
    RingLogBuffer,
} from "cap:logger";

pub fn demoObservability() {
    const tp = TraceParent.parse("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01").?;
    let trace_buf: [32]u8 = [0; 32];
    let span_buf: [16]u8 = [0; 16];

    let logger = StructuredLogger.initNamed("checkout", LogLevel.Info, LogFormat.Json)
        .withStr("env", "prod")
        .withTrace(tp.copyTraceId(trace_buf[0..32]), tp.copySpanId(span_buf[0..16]));

    * = logger.info("user authenticated", [
        LogField.str("user_id", "u_42"),
        LogField.str("password", "secret_pw"), // 自动脱敏为 "[REDACTED]"
    ]);

    let span = Span.start("db.fetch", tp);
    * = span.setAttrStr("db.system", "postgres");
    span.setStatus(SpanStatus.Ok, "");
    span.end();

    let json_buf: [1024]u8 = [0; 1024];
    const span_json = span.toJson(json_buf[0..1024]);
    * = span_json;
}
```

### 2. 源码级诊断日志（向后兼容）

```typescript
import { LogLevel, Loc, Range, Source, DiagMsg, Log } from "cap:logger";

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

    let plain_buf: [2048]u8 = [0; 2048];
    const plain = log.formatAll(plain_buf[0..2048], false);
    * = plain;
}
```

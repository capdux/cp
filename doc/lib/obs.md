# CP 标准库：可观测性、指标、SIEM 日志、诊断报告与 eBPF BTF 手册 (`cap:obs` / `cap.obs`)

## 一、 架构总览与设计准则

`cap:obs` 是 CP 语言官方内置的纯原生零 C 依赖可观测性、指标、安全日志、诊断报告与内核类型追踪标准库，覆盖：
1. **OpenMetrics / Prometheus 文本指标格式**（`openmetrics.cp`）
2. **SIEM 安全事件与 HTTP 访问日志**（`cef.cp`, `leef.cp`, `clf.cp`）
3. **静态分析诊断、CI 测试报告与 LLM 评测门禁**（`sarif.cp`, `junit.cp`, `tap.cp`, `eval_metrics.cp`, `eval_suite.cp`）
4. **Linux eBPF BTF 内核类型图解析**（`btf.cp`）

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:obs"` 与 `import cap from "cap"; cap.obs.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 核心职责 |
| :--- | :--- |
| `lib/obs/openmetrics.cp` | OpenMetrics / Prometheus Exposition 文本指标解析器（`MetricsDocument`, `parse`） |
| `lib/obs/cef.cp` | ArcSight CEF Rev 25 SIEM 日志解析、转义解码与头部格式化 |
| `lib/obs/leef.cp` | IBM QRadar LEEF 1.0/2.0 SIEM 日志解析与属性遍历 |
| `lib/obs/clf.cp` | NCSA Common & Combined HTTP 访问日志流式解析与聚合统计 |
| `lib/obs/sarif.cp` | OASIS SARIF 2.1.0 静态分析结果解析与级别统计 |
| `lib/obs/junit.cp` | JUnit XML CI 测试套件与用例结果解析汇总 |
| `lib/obs/tap.cp` | TAP v12/v13/v14（Test Anything Protocol）事件流解析与统计 |
| `lib/obs/eval_metrics.cp` | LLM / AI Agent 评测打分器（Exact/Token F1/JSON 字段匹配/`pass@k`） |
| `lib/obs/eval_suite.cp` | 评测报告聚合与 CI 回归门禁对比（`EvalReport`, `RegressionDiff`） |
| `lib/obs/btf.cp` | Linux 内核 `.BTF`（BPF Type Format）类型图与成员布局零分配解析器 |
| `lib/obs/telemetry_export.cp` | 零堆分配 16 桶对数延迟直方图（`LatencyBucketHistogram`、p50/p95/p99 与 Prometheus 文本导出）及 OTLP/HTTP JSON 链路 Span 批导出器（`OtlpSpanRecord`, `OtlpSpanBatch`） |
| `lib/obs/index.cp` | `cap:obs` 与 `cap.obs` 统一零冲突公开导出入口 |

---

## 二、 核心 API 索引

- **OpenMetrics（`openmetrics.cp`）**：`MetricType`, `Label`, `Sample`, `MetricFamily`, `MetricsDocument`, `parse`
- **SIEM & Access Logs（`cef.cp`, `leef.cp`, `clf.cp`）**：`CEF_PREFIX`, `CefEvent`, `parseCefLine`, `findCefExt`, `LEEF_PREFIX`, `LeefEvent`, `parseLeefLine`, `findLeefAttr`, `ClfEntry`, `ClfStats`, `parseClfLine`, `summarizeClf`
- **Static Analysis & CI Reports（`sarif.cp`, `junit.cp`, `tap.cp`）**：`SarifLevel`, `SarifHeader`, `SarifRun`, `SarifResult`, `summarizeSarif`, `JunitOutcome`, `JunitSuite`, `JunitCase`, `summarizeJunit`, `TapEventKind`, `TapDirective`, `TapEvent`, `summarizeTap`
- **LLM Eval & Regression Gate（`eval_metrics.cp`, `eval_suite.cp`）**：`EvalError`, `MatchMode`, `EvalScorer`, `EvalCase`, `EvalSummary`, `RegressionDiff`, `EvalReport`
- **eBPF BTF（`btf.cp`）**：`BTF_MAGIC`, `BtfHeader`, `BtfType`, `BtfIntInfo`, `BtfArray`, `BtfMember`, `BtfTypeIter`, `BtfSection`, `isBtf`, `parseBtf`, `parseBtfInt`, `parseBtfArray`, `btfMemberAt`

---

## 三、 使用示例

```typescript
import cap from "cap";
import {
    parse,
    parseCefLine,
    EvalScorer,
    MatchMode,
} from "cap:obs";

pub fn demoObs(alloc: std.mem.Allocator): !void {
    // 1. OpenMetrics 指标文本解析（cap.obs 动态挂载等价）
    let doc = cap.obs.parse(alloc, "# TYPE http_requests counter\nhttp_requests{code=\"200\"} 42\n")?;
    defer doc.deinit();

    // 2. ArcSight CEF SIEM 日志解析
    const ev = parseCefLine("CEF:0|Security|threatmanager|1.0|100|worm stopped|10|src=10.0.0.1");
    * = ev;

    // 3. LLM 评测打分
    const score = EvalScorer.exactMatch("cp", " cp ", MatchMode.ExactTrimmed);
    * = score;
}
```

### 零堆分配对数延迟直方图与 OTLP JSON Span 批导出示例 (`telemetry_export.cp`)

```cp
import {
    LatencyBucketHistogram,
    OtlpSpanRecord,
    OtlpSpanBatch,
} from "cap:obs";

pub fn demoTelemetryExport() {
    // 1. 16 桶对数延迟直方图 + 分位数估算 + Prometheus 文本导出
    let hist = LatencyBucketHistogram.init();
    hist.observe(120);
    hist.observe(850);
    hist.observe(4200);
    * = hist.quantile(0.95);

    let prom_buf: [1024]u8 = [0; 1024];
    if (n_prom := hist.renderPrometheus("agent_step_latency_us", prom_buf[0..])) {
        * = prom_buf[0..n_prom];
    }

    // 2. 固定槽位 OTLP/HTTP JSON 链路 Span 批导出
    let slots: [4]OtlpSpanRecord = [OtlpSpanRecord{}; 4];
    let batch = OtlpSpanBatch.init(slots[0..]);
    * = batch.push(OtlpSpanRecord{
        trace_id_hex: "4bf92f3577b34da6a3ce929d0e0e4736",
        span_id_hex: "00f067aa0ba902b7",
        parent_span_id_hex: "",
        name: "POST /v1/chat",
        start_us: 1000000,
        end_us: 1004200,
        status_code: 200,
        is_error: false,
    });

    let otlp_buf: [1024]u8 = [0; 1024];
    if (n_otlp := batch.renderOtlpJson("cp-agent-gateway", otlp_buf[0..])) {
        * = otlp_buf[0..n_otlp];
    }
}
```

### 零堆分配结构化日志与 JSONL / Logfmt 双格式渲染示例 (`structured_log.cp`)

面向生产服务与 AI Agent 链路可观测性的 100% 纯 CP 零堆分配结构化日志门面：

- **导出符号**：`OBS_LOG_MAX_FIELDS`、`ObsLogLevel`（`Trace | Debug | Info | Warn | Error | Fatal`）、`ObsLogFieldKind`（`Str | Int | Float | Bool`）、`ObsLogField`、`ObsLogRecord`（含 `service`/`trace_id`/`span_id` 链路关联与最多 16 个结构化字段）、`ObsLogFormat`（`JsonLines | Logfmt`）、`ObsLoggerConfig`、`ObsLoggerStats`、`ObsStructuredLogger`（`init`, `shouldLog`, `formatRecord`, `emit`, `stats`, `resetStats`）。

```cp
import {
    ObsLogLevel,
    ObsLogField,
    ObsLogRecord,
    ObsLogFormat,
    ObsLoggerConfig,
    ObsStructuredLogger,
} from "cap:obs";

pub fn demoStructuredLogger() {
    let logger = ObsStructuredLogger.init(ObsLoggerConfig{
        min_level: ObsLogLevel.Info,
        format: ObsLogFormat.JsonLines,
        default_service: "cp-agent-worker",
        append_newline: true,
    });

    let rec = ObsLogRecord{
        timestamp_ms: 1732250000123,
        level: ObsLogLevel.Info,
        trace_id: "4bf92f3577b34da6a3ce929d0e0e4736",
        span_id: "00f067aa0ba902b7",
        message: "tool call completed",
    };
    * = rec.addField(ObsLogField.str("tool", "search_docs"));
    * = rec.addField(ObsLogField.int("latency_ms", 42));
    * = rec.addField(ObsLogField.float("score", 0.9625));
    * = rec.addField(ObsLogField.boolean("cached", true));

    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];
    if (n := logger.emit(rec, out_s)) {
        * = out_buf[0..n];
    }
    * = logger.stats();
}
```

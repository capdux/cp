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
| `lib/obs/structured_log.cp` | 零堆分配 JSONL / Logfmt 结构化日志与链路追踪关联引擎（`ObsStructuredLogger`） |
| `lib/obs/anomaly_detector.cp` | 零堆分配流式 Welford/EWMA 在线异常检测、连续违约防抖与冷却防风暴控制器（`OnlineAnomalyDetector`） |
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

### 零堆分配流式在线异常检测与告警防抖示例 (`anomaly_detector.cp`)

面向微服务延迟突刺与 AI Agent 成本/错误率监控的 100% 纯 CP 零堆分配流式异常检测器：

- **导出符号**：`AnomalyConfig`（`warmup_samples`, `z_threshold`, `ewma_alpha`, `consecutive_breaches`, `cooldown_ms`, `min_stddev_eps`）、`AnomalySeverity`（`Normal | Warmup | Warning | AlertFired | SuppressedCooldown`）、`AnomalyEval`、`AnomalyDetectorStats`、`OnlineAnomalyDetector`（`init`, `observe`, `baselineMean`, `baselineStddev`, `ewma`, `stats`, `reset`）。

```cp
import {
    AnomalyConfig,
    AnomalySeverity,
    OnlineAnomalyDetector,
} from "cap:obs";

pub fn demoOnlineAnomalyDetector() {
    let det = OnlineAnomalyDetector.init(AnomalyConfig{
        warmup_samples: 5,
        z_threshold: 3.0,
        ewma_alpha: 0.25,
        consecutive_breaches: 2,
        cooldown_ms: 10000,
    }) ?? return;

    // 1. 预热期（静默学习 Welford 基线，不误报）
    const base: [5]f64 = [10.0, 11.0, 9.0, 10.5, 9.5];
    let i: usize = 0;
    while (i < 5) : (i += 1) {
        * = det.observe(base[i], (i as i64) * 1000);
    }

    // 2. 连续 2 次突刺：第 1 次 Warning 防抖，第 2 次 AlertFired，冷却期内第 3 次 SuppressedCooldown
    const e1 = det.observe(50.0, 5000); // AnomalySeverity.Warning
    const e2 = det.observe(52.0, 6000); // AnomalySeverity.AlertFired
    const e3 = det.observe(55.0, 7000); // AnomalySeverity.SuppressedCooldown
    * = e1; * = e2; * = e3;
}
```

### 零堆分配 HDR 对数线性高精度延迟直方图与协同遗漏校正示例 (`hdr_histogram.cp`)

面向高并发微服务、存储引擎与 AI Agent 推理网关的 100% 纯 CP 零动态堆分配 HdrHistogram 风格延迟直方图内核（恒定 1920 桶内联存储，覆盖 `[0, 34_359_738_367]` 动态范围，相对误差 `<= 1.5625%`）：

- **几何常量**：`HDR_SUB_BUCKETS` (`64`)、`HDR_NUM_SCALES` (`28`)、`HDR_TOTAL_BUCKETS` (`1920`)、`HDR_MAX_SUPPORTED_VALUE` (`34_359_738_367`)。
- **核心类型**：
  - `HdrQuantileSummary`：`{ total_count, min_val, max_val, mean_f64, p50, p90, p95, p99, p999 }`。
  - `HdrHistogram`：
    - `HdrHistogram.init(max_trackable_val)`：初始化直方图（`0` 或超上限自动夹紧至 `HDR_MAX_SUPPORTED_VALUE`）。
    - `value_to_bucket(val)` / `bucket_upper_bound(bucket_idx)`：纯位运算 O(1) 对数线性映射（`[0, 127]` 零误差精确映射，更高量级保守上界 `<= 1.5625%` 相对误差）。
    - `record(val)` / `record_n(val, count)`：O(1) 写入；超 `max_trackable_val` 时截断入桶并累计 `clamped_count` 后返回 `false`。
    - `record_with_expected_interval(val, expected_interval)`：Coordinated Omission 协同遗漏校正，当 `val > expected_interval` 时自动按步长回填被阻塞的隐形高延迟样本。
    - `value_at_quantile(q)` / `summarize()`：计算任意分位点与 `P50/P90/P95/P99/P99.9` 汇总。
    - `merge_from(other)`：跨线程/分片直方图无损聚合（要求 `max_trackable_val` 一致）。
    - `reset()`：清零复用。

```cp
import {
    HdrHistogram,
} from "cap:obs";

pub fn demoHdrHistogram() {
    let h = HdrHistogram.init(60_000_000); // 追踪最高 60 秒（微秒粒度）
    // 常规快速请求：1000us
    * = h.record_n(1_000, 990);
    // 1 次 100ms 停顿，预期到达间隔 10ms -> Coordinated Omission 自动回填 9 条隐形排队样本
    * = h.record_with_expected_interval(100_000, 10_000);

    const s = h.summarize();
    // s.p50 ~ 1000us, s.p99 精确暴露出被阻塞的高尾延迟
    * = s.p99;
}
```

### 零堆分配 OpenTelemetry 尾部链路采样器与决策引擎 (`trace_sampler.cp`)

面向高吞吐微服务与百万级 AI Agent 分布式链路追踪（对标 OpenTelemetry Collector / Jaeger Tail-Based Sampling）的 100% 纯 CP 零动态堆分配尾部采样决策引擎（内联 128 槽位在途 Trace 聚合池）：

- **常量与枚举**：
  - `TAIL_SAMPLER_MAX_TRACES` (`128`)
  - `TailSampleReason`（`dropped | kept_error | kept_slow_latency | kept_priority_tag | kept_probabilistic | dropped_budget_cap`）
- **核心类型**：
  - `TailSamplerConfig`（`slow_threshold_us`、`baseline_sample_bps` [`0..=10000`]、`decision_wait_ms`、`max_kept_per_window`、`rate_window_ms`；提供 `defaults()` 与 `isValid()`）
  - `ActiveTraceSlot`、`TailSampleDecision`、`TailSamplerStats`
- **核心引擎 `TailTraceSampler`**：
  - `init(cfg): TailTraceSampler?`：校验配置合法性并初始化零堆 128 槽位尾部采样器。
  - `ingest_span(*_, trace_id_lo, trace_id_hi, duration_us, is_error, is_priority, is_root, now_ms): bool`：按 128 位 TraceID 聚合多 Span 属性（拒绝全零 TraceID），满 128 槽时自动驱逐最旧在途槽位并累计 `evicted_overflow`。
  - `flush_ready_decisions(*_, now_ms, out_decisions): usize`：对 `root_finished == true` 或超时 `now_ms - first_seen_ms >= decision_wait_ms` 的就绪链路执行四级优先级尾部决策（错误优先 -> 慢延迟优先 -> 高优标签优先 -> SplitMix64 确定性基点概率采样），并通过每 `rate_window_ms` 窗口 `max_kept_per_window` 预算熔断防止错误风暴打爆下游存储；输出缓冲满时暂停出队以免丢失决策。
  - `stats(): TailSamplerStats`：返回在途数、累计评估数、各类保留/丢弃/预算熔断与满池驱逐统计。

```cp
import {
    TailSamplerConfig,
    TailSampleDecision,
    TailTraceSampler,
} from "cap:obs";

pub fn demoTailTraceSampler() {
    let cfg = TailSamplerConfig.defaults();
    cfg.slow_threshold_us = 200_000; // 200ms
    cfg.baseline_sample_bps = 1000;  // 10% 基线采样
    cfg.max_kept_per_window = 50;

    let sampler = TailTraceSampler.init(cfg) ?? return;

    // 同一 Trace 的子 Span 报错，随后 Root Span 结束 -> 尾部 100% 保留 (kept_error)
    * = sampler.ingest_span(0x1001, 0xAAAA, 12_000, true, false, false, 1000);
    * = sampler.ingest_span(0x1001, 0xAAAA, 45_000, false, false, true, 1045);

    let decisions: [8]TailSampleDecision = [TailSampleDecision{}; 8];
    let dec_s: []TailSampleDecision = decisions[0..];
    const n = sampler.flush_ready_decisions(1050, dec_s);
    * = n; // 1 (decisions[0].keep == true, reason == TailSampleReason.kept_error)
}
```

### 零分配 Google SRE 多窗口多燃烧率 SLO 错误预算与告警引擎 (`slo_burn_rate.cp`)

面向云原生服务、API 网关与 AI Agent 推理集群的 100% 纯 CP 零动态堆分配 Google SRE Workbook 第 5 章多窗口多燃烧率（Multi-Window Multi-Burn-Rate）SLO 错误预算与告警内核（固定 360 个 1 分钟环形桶）：

- **常量与枚举**：
  - `SLO_RING_MINUTES` (`360`，6 小时 1 分钟粒度环形桶)
  - `SloAlertSeverity`（`ok | ticket_slow_burn | page_high_burn | page_critical_burn`）
- **核心类型**：
  - `SloBurnConfig`（`target_availability_f64`、`critical_burn_rate` [默认 `14.4`]、`high_burn_rate` [默认 `6.0`]、`slow_burn_rate` [默认 `1.0`]、`min_samples_short_win` [默认 `20`]；提供 `defaultsThreeNines()` 与 `isValid()`）
  - `SloWindowMetrics`（`good_events`、`bad_events`、`total_events`、`error_rate_f64`、`burn_rate_f64`）
  - `SloBurnEvaluation`（`severity`、`win_5m`、`win_30m`、`win_1h`、`win_6h`、`error_budget_consumed_ratio`、`hours_to_budget_exhaustion`）
- **核心监控器 `SloBurnMonitor`**：
  - `init(cfg): SloBurnMonitor?` / `reset(*_)`
  - `record_events(*_, minute_epoch, good_count, bad_count): bool`：自动推进并清零过期分钟桶，支持窗口内乱序补录，安全拒绝早于当前窗口 `>= 360` 分钟的陈旧事件。
  - `window_metrics(now_minute, span_minutes): SloWindowMetrics`：精确统计指定回溯分钟窗口的好/坏样本数、错误率与燃烧率。
  - `evaluate(now_minute): SloBurnEvaluation`：严格执行 Google SRE 双窗防抖（`page_critical_burn` 需 1h+5m 双窗 >= 14.4x、`page_high_burn` 需 6h+30m 双窗 >= 6x、`ticket_slow_burn` 需 6h+30m 双窗 >= 1x，且满足 `min_samples_short_win` 防冷启动误报），故障恢复后短窗归零即可快速消警。

```cp
import {
    SloAlertSeverity,
    SloBurnConfig,
    SloBurnMonitor,
} from "cap:obs";

pub fn demoSloBurnMonitor() {
    let mon = SloBurnMonitor.init(SloBurnConfig.defaultsThreeNines()) ?? return;

    // 突发 2% 错误率（对于 99.9% SLO 相当于 20x 燃烧率 > 14.4x）
    let m: u64 = 1000;
    while (m < 1010) : (m += 1) {
        * = mon.record_events(m, 980, 20);
    }

    const ev = mon.evaluate(1009);
    if (ev.severity == SloAlertSeverity.page_critical_burn) {
        * = ev.hours_to_budget_exhaustion; // ~36.0h (720 / 20)
    }
}
```

### 零分配 OpenTelemetry / Prometheus 直方图 Exemplar 蓄水池采样器 (`exemplar_reservoir.cp`)

面向云原生 Prometheus / OpenTelemetry 指标与链路追踪（Trace/Span）打通的 100% 纯 CP 零动态堆分配直方图 Exemplar 蓄水池采样器：

- **常量与枚举**：
  - `EXEMPLAR_MAX_BOUNDS = 31`、`EXEMPLAR_MAX_BUCKETS = 32`（含 `+Inf` 尾桶）、`EXEMPLAR_MAX_RESERVOIR_SLOTS = 32`、`EXEMPLAR_FLAG_SAMPLED = 0x01`
  - `ExemplarStrategy`（`aligned_histogram_bucket` 每桶独立留存确保 P99 长尾样本不丢 | `simple_fixed_size` 确定性 SplitMix64 Algorithm R 均匀蓄水池采样）
- **数据模型与格式化**：
  - `ExemplarRecord`（`value`, `timestamp_ns`, `trace_id_hi`, `trace_id_lo`, `span_id`, `sampled_flags`, `bucket_idx`, `occupied`）
  - `ExemplarBucketSummary`、`ExemplarReservoirStats`
  - `formatExemplarOpenMetrics(ex, out_buf: *[]u8): usize?`：按 OpenMetrics 1.0 规范输出 `# {trace_id="<32-hex>",span_id="<16-hex>"} <val> <ts>`，缓冲区不足或非法记录返回 `null`。
- **核心引擎 `AlignedHistogramExemplarReservoir`**：
  - `initAligned(bounds, require_sampled_flag)` / `initSimpleFixed(bounds, reservoir_size, rng_seed, require_sampled_flag)`
  - `findBucketIndex(value)`（O(log B) 二分查找）
  - `observe(*_, value, timestamp_ns, trace_id_hi, trace_id_lo, span_id, sampled_flags): bool`：自动拦截 NaN/Inf，过滤 `trace_id == 0` / `span_id == 0` 或未采样标志（仍累计直方图桶计数与总和）。
  - `evictOlderThan(*_, cutoff_ns)` / `collectAndReset(*_, out_exemplars)` / `getBucketSummary(bucket_idx)` / `stats()`

```cp
import {
    EXEMPLAR_FLAG_SAMPLED,
    ExemplarRecord,
    formatExemplarOpenMetrics,
    AlignedHistogramExemplarReservoir,
} from "cap:obs";

pub fn demoExemplarReservoir() {
    const bounds: [4]f64 = [0.05, 0.25, 1.0, 5.0];
    let res = AlignedHistogramExemplarReservoir.initAligned(bounds[0..], true) ?? return;

    * = res.observe(0.02, 1_700_000_000_000_000_000, 0x01, 0x02, 0xAA, EXEMPLAR_FLAG_SAMPLED);
    * = res.observe(6.25, 1_700_000_001_500_000_000, 0xDEADBEEF, 0xCAFEBABE, 0x1234, EXEMPLAR_FLAG_SAMPLED);

    let out: [8]ExemplarRecord = [ExemplarRecord{}; 8];
    let out_s: []ExemplarRecord = out[0..];
    const n = res.collectAndReset(*out_s); // 2 (桶 0 与 +Inf 慢请求桶各留存 1 条)
    if (n > 0) {
        let txt: [128]u8 = [0; 128];
        let txt_s: []u8 = txt[0..];
        * = formatExemplarOpenMetrics(out[n - 1], *txt_s);
    }
}
```

---

## 收敛子模块深度文档

- [btf.md](obs/btf.md)
- [cef.md](obs/cef.md)
- [clf.md](obs/clf.md)
- [eval.md](obs/eval.md)
- [junit.md](obs/junit.md)
- [leef.md](obs/leef.md)
- [openmetrics.md](obs/openmetrics.md)
- [sarif.md](obs/sarif.md)
- [tap.md](obs/tap.md)

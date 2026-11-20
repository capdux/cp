# CP 标准库：可观测性指标与 Prometheus/OpenMetrics 引擎 (`cap:metrics` / `cap.metrics`)

## 一、 架构总览与设计准则

`cap:metrics` 是 CP 语言官方内置的纯原生可观测性指标与 Prometheus/OpenMetrics 导出库，专为百万级 AI Agent 并发运行时与云原生服务 `/metrics` 端点采集打造。核心组件分为三层：

1. **原子原语与校验器 (`lib/metrics/primitives.cp`)**：
   - `Counter`：基于 `std.atomic.Value(u64)` 的无锁单调递增计数器（`inc`、`add`、`get`、`reset`）；
   - `Gauge`：基于 IEEE-754 位模式 CAS 循环的原子 `f64` 仪表盘（`set`、`inc`、`dec`、`add`、`sub`、`get`、`reset`）；
   - `MetricLabel` & `MetricNameValidator`：严格校验 Prometheus 指标名 `[a-zA-Z_:][a-zA-Z0-9_:]*` 与标签名 `[a-zA-Z_][a-zA-Z0-9_]*`。
2. **累计桶直方图与分位数估计 (`lib/metrics/histogram.cp`)**：
   - `MetricsHistogram`：内置默认 HTTP 延迟桶（`0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0`）与自定义严格单调递增边界桶，提供 `observe`、`bucketCounts`、`sum`、`count` 与 Prometheus 线性插值分位数估算 `estimateQuantile(q)`（支持 p50/p95/p99）。
3. **指标注册表与多格式导出器 (`lib/metrics/registry.cp`)**：
   - `MetricsRegistry`：线程安全、指针稳定、零悬垂深拷贝的统一注册中心；
   - `exportPrometheus(alloc)`：严格输出 **Prometheus Text Exposition Format 0.0.4**（同族 `# HELP`/`# TYPE` 合并、标签值 `\\`/`\"`/`\n` 转义、直方图 `_bucket{...,le="..."}`、`le="+Inf"`、`_sum`、`_count`）；
   - `exportJson(alloc)`：导出合法 RFC 8259 结构化 JSON 快照，便于调试与 AI Agent 运行时自省。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:metrics"` 与 `import cap from "cap"; cap.metrics.*` 编译期按需惰性挂载。

---

## 二、 核心 API 速查表

### 1. 原子原语与校验器 (`primitives.cp`)

| 符号 / 方法 | 说明 |
| :--- | :--- |
| `MetricsError` | `InvalidMetricName`, `InvalidLabelName`, `DuplicateMetric`, `InvalidBounds`, `InvalidQuantile`, `InvalidValue`, `OutOfMemory` |
| `MetricLabel` | `{ key: []const u8, value: []const u8 }`，`MetricLabel.init(key, value)` |
| `MetricNameValidator` | `isValidMetricName(name)`、`isValidLabelName(name)`、`validateMetricName`、`validateLabelName`、`validateLabels` |
| `Counter` | `init()`、`inc(*_)`、`add(*_, delta: u64)`、`get(): u64`、`reset(*_)` |
| `Gauge` | `init()`、`initWith(val)`、`set(*_, val: f64)`、`inc(*_)`、`dec(*_)`、`add(*_, delta: f64)`、`sub(*_, delta: f64)`、`get(): f64`、`reset(*_)` |

### 2. 直方图与分位数估计 (`histogram.cp`)

| 符号 / 方法 | 说明 |
| :--- | :--- |
| `DEFAULT_HTTP_BUCKETS` | `[11]f64 = [0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0]` |
| `MetricsHistogram.init(alloc, bounds)` | 初始化直方图（`bounds` 为空时自动使用 `DEFAULT_HTTP_BUCKETS`） |
| `MetricsHistogram.initDefault(alloc)` | 使用默认 HTTP 延迟桶初始化直方图 |
| `observe(*_, value: f64)` | 观测样本并更新累计桶计数、`_sum` 与 `_count`（自动忽略 `NaN`） |
| `bucketCounts()` / `bounds()` | 返回累计桶计数切片（末位为 `+Inf`）与有限上界切片 |
| `sum()` / `count()` / `reset(*_)` | 返回累计和、样本总数，或清零计数 |
| `estimateQuantile(q: f64)` | 基于累计桶线性插值估算 `q` 分位数（空直方图返回 `0.0`） |
| `estimateQuantileChecked(q: f64)` | 严格校验 `q ∈ [0.0, 1.0]` 且非 `NaN` 的分位数估算 |

### 3. 注册表与导出器 (`registry.cp`)

| 符号 / 方法 | 说明 |
| :--- | :--- |
| `MetricsRegistry.init(alloc)` / `deinit(*_)` | 创建并管理注册表生命周期（自动释放全部内部状态与深拷贝字符串） |
| `registerCounter(*_, name, help, labels)` | 校验并注册计数器，返回稳定绑定句柄 `Counter` |
| `registerGauge(*_, name, help, labels)` | 校验并注册仪表盘，返回稳定绑定句柄 `Gauge` |
| `registerHistogram(*_, name, help, labels, bounds)` | 校验并注册直方图（拒绝保留标签 `"le"` 与非法边界） |
| `exportPrometheus(alloc)` | 导出 Prometheus Text Exposition 0.0.4 文本（调用方负责 `alloc.free`） |
| `exportJson(alloc)` | 导出结构化 JSON 快照（调用方负责 `alloc.free`） |

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import { MetricsRegistry, MetricLabel } from "cap:metrics";

fn serveMetrics(): !void {
    const alloc = std.heap.page_allocator;
    let reg = MetricsRegistry.init(alloc);
    defer reg.deinit();

    const labels = [MetricLabel.init("agent", "planner"), MetricLabel.init("status", "ok")];
    let reqs = reg.registerCounter("agent_requests_total", "Total agent requests", &labels)?;
    let inflight = reg.registerGauge("agent_inflight_tasks", "Active inflight tasks", &[])?;
    let latency = reg.registerHistogram("agent_step_seconds", "Step latency", &labels, &[])?;

    reqs.inc();
    inflight.set(3.0);
    latency.observe(0.042);

    const prom_text = reg.exportPrometheus(alloc)?;
    defer alloc.free(prom_text);

    // 等价动态挂载用法：cap.metrics.*
    const p95 = latency.estimateQuantile(0.95);
    let standalone = cap.metrics.Counter.init();
    standalone.add(10);
    * = p95;
}
```

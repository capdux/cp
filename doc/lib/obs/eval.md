# CP 标准库：AI Agent 自动化评测、Pass@k 估算与 CI 回归门禁 (`cap:eval`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

## 一、 架构总览与设计准则

`cap:eval` 是 CP 语言官方内置的工业生产级纯 CP AI Agent 与 LLM 评测标准库，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，可与 `cap:json`、`cap:text` 无缝协作，既可通过 `import { ... } from "cap:obs"` 静态导入，也支持 `import cap from "cap"; cap.eval.*` 动态挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                AI Agent 自动化评测与 CI 回归门禁引擎 (cap:eval)            │
├───────────────────────┬──────────────────────────┬──────────────────────┤
│ 确定性与语义评分器     │ 无偏采样与结构化校验      │ 套件聚合与 CI 门禁    │
│ EvalScorer.exactMatch │ EvalScorer.passAtK       │ EvalReport.summarize │
│ containsAll / tokenF1 │ (迭代乘积防溢出公式)      │ compareBaseline      │
│ forbiddenNone         │ jsonFieldMatch (cap:json)│ toMarkdown / toJson  │
└───────────────────────┴──────────────────────────┴──────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/eval/metrics.cp` | `// Source: cp/lib/eval/metrics.cp (...)` | `EvalError`、`MatchMode`、`EvalScorer`（多模式匹配、关键词覆盖、违禁词拦截、SQuAD `tokenF1`、`jsonFieldMatch`、无偏 `passAtK`） |
| `lib/eval/suite.cp` | `// Source: cp/lib/eval/suite.cp (...)` | `EvalCase`、`EvalSummary`、`RegressionDiff`、`EvalReport`（加权评分、精确 P95 延迟、Token/Cost 统计、基线回归门禁、Markdown/JSON 导出） |
| `lib/eval/index.cp` | `// Source: cp/lib/eval/index.cp (...)` | `cap:eval` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、匹配模式与评分器（`metrics.cp`）

- **`EvalError`**：`InvalidInput`、`InvalidThreshold`、`EmptyDataset`、`DivisionByZero`、`OutOfMemory`。
- **`MatchMode`**：`Exact`、`ExactTrimmed`、`CaseInsensitive`、`Contains`、`Prefix`、`Suffix`。
- **`EvalScorer`**（静态工具类）：
  - `exactMatch(expected: string, actual: string, mode: MatchMode): f64`：按指定模式比对，命中返回 `1.0`，否则返回 `0.0`。
  - `containsAll(actual: string, keywords: []const string): f64`：计算 `keywords` 在 `actual` 中的命中比例（`0.0..1.0`；空列表返回 `1.0`）。
  - `forbiddenNone(actual: string, forbidden: []const string): f64`：出现任一非空违禁词返回 `0.0`，否则返回 `1.0`。
  - `tokenF1(alloc: std.mem.Allocator, expected: string, actual: string): f64 ! EvalError`：按 ASCII 空白/标点分词并小写归一化，基于多重集交集计算经典 SQuAD Precision / Recall / F1（`0.0..1.0`）。
  - `jsonFieldMatch(alloc: std.mem.Allocator, expected_json: string, actual_json: string, field_path: string): f64 ! EvalError`：基于 `cap:json` 解析双方 JSON，支持点路径（如 `"user.profile.role"`）与 RFC 6901 JSON Pointer（如 `"/items/0/id"`），结构相等返回 `1.0`，否则 `0.0`；非法 JSON 返回 `EvalError.InvalidInput`。
  - `passAtK(n: usize, c: usize, k: usize): f64 ! EvalError`：Codex 论文无偏估计 `1 - C(n-c, k) / C(n, k)`，采用迭代乘积 `prod((n-c-i)/(n-i))` 避免大组合数阶乘溢出；要求 `n >= k >= 1` 且 `0 <= c <= n`。

### 2. 评测套件聚合与 CI 回归门禁（`suite.cp`）

- **`EvalCase`**：`id`、`category`、`score`、`passed`、`latency_ms`、`tokens_in`、`tokens_out`、`cost_usd`、`weight`（默认 `1.0`）。
- **`EvalSummary`**：`total_cases`、`passed_cases`、`pass_rate`、`weighted_score`、`mean_latency_ms`、`p95_latency_ms`、`total_tokens`、`total_cost_usd`。
- **`RegressionDiff`**：`pass_rate_delta`、`score_delta`、`latency_p95_delta_ms`、`cost_delta_usd`、`regressed`、`reason`。
- **`EvalReport`**：
  - `init(alloc, suite_name): EvalReport ! EvalError` / `deinit(*_)`
  - `recordCase(*_, c: EvalCase): void ! EvalError`
  - `summarize(): EvalSummary ! EvalError`（空数据集返回 `EvalError.EmptyDataset`，总权重为 0 返回 `EvalError.DivisionByZero`）
  - `compareBaseline(baseline_summary, max_score_drop, max_p95_increase_ms): RegressionDiff ! EvalError`
  - `toMarkdown(): string ! EvalError` / `toJson(): string ! EvalError`（返回调用方持有的切片，使用完毕需 `alloc.free`）

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { EvalScorer, MatchMode, EvalReport, EvalCase } from "cap:obs";
import cap from "cap";

pub fn demoEval(): ! {
    const alloc = std.heap.page_allocator;

    const f1 = EvalScorer.tokenF1(alloc, "The quick brown fox", "the fast brown dog")?;
    const p10 = EvalScorer.passAtK(100, 1, 10)?;

    let report = EvalReport.init(alloc, "agent-nightly")?;
    defer report.deinit();

    report.recordCase(EvalCase{
        id: "qa-001",
        category: "rag",
        score: f1,
        passed: f1 >= 0.5,
        latency_ms: 42.5,
        tokens_in: 120,
        tokens_out: 30,
        cost_usd: 0.0004,
        weight: 1.0,
    })?;

    const summary = report.summarize()?;
    const md = report.toMarkdown()?;
    defer alloc.free(md);

    const exact = cap.eval.EvalScorer.exactMatch("OK", " ok ", MatchMode.ExactTrimmed);
    * = p10;
    * = summary;
    * = exact;
}
```

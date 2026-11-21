# `cap:trx` — 零分配 Visual Studio / .NET `.trx` 测试结果解析标准库

`cap:trx` 是纯 CP、零堆分配、零外部依赖的 Microsoft Visual Studio / .NET（`dotnet test --logger trx`）`.trx` XML 测试结果解析与汇总库。

---

## 快速导入

```typescript
import {
    TrxOutcome,
    TrxRunHeader,
    TrxResult,
    TrxSummary,
    TrxResultIter,
    isTrx,
    parseTrxHeader,
    parseTrxSummary,
} from "cap:trx";
```

---

## 核心 API

- **枚举**：`TrxOutcome: u8`（`passed`, `failed`, `not_executed`, `inconclusive`, `timeout`, `aborted`, `error_outcome`, `unknown`）
- **嗅探与运行头**：`isTrx(data): bool`（跳过可选 UTF-8 BOM / XML 声明 / 注释匹配 `<TestRun`）、`parseTrxHeader(data): TrxRunHeader?`
- **结果迭代器**：`TrxResultIter.init(data)` / `next(*_): TrxResult?`（支持自闭合与含 `<Output>` 子节点，提取 `test_name`, `execution_id`, `test_id`, `computer_name`, `duration`, `durationMs(): f64`, `outcome`, `error_message`, `stack_trace`, `stdout_text`）
- **汇总统计**：`parseTrxSummary(data): TrxSummary?`（优先解析 `<ResultSummary>`/`<Counters>`，缺失 `<Counters>` 时自动回退单遍扫描 `<UnitTestResult>` 实算；附 `allPassed(): bool`）

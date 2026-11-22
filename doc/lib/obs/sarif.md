# `cap:sarif` — 零分配 OASIS SARIF 2.1.0 静态代码分析结果解析标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:obs`，导入请使用 `import { ... } from "cap:obs"`。

`cap:sarif` 是纯 CP、零堆分配、零外部依赖的 OASIS SARIF v2.1.0 静态分析报告解析与汇总库，广泛适用于 GitHub Code Scanning、CodeQL、Semgrep、Clippy 与 ESLint 产出的 SARIF JSON 报告。

---

## 快速导入

```typescript
import {
    SarifLevel,
    SarifHeader,
    SarifRun,
    SarifResult,
    SarifStats,
    SarifRunIter,
    SarifResultIter,
    isSarif,
    parseSarifHeader,
    summarizeSarif,
} from "cap:obs";
```

---

## 核心 API

- **严重级别枚举**：`SarifLevel: u8`（`none=0`, `note=1`, `warning=2`, `error_level=3`；遵循 SARIF 2.1.0 规范缺省回退 `warning`）
- **嗅探与顶层头**：`isSarif(data): bool`、`parseSarifHeader(data): SarifHeader?`（提取 `version`, `schema_uri`, `runs_slice`）
- **双层零分配迭代器**：
  - `SarifRunIter.init(hdr.runs_slice)` -> `SarifRun`（`tool_name`, `tool_version`, `info_uri`, `results_slice`, `results()`）
  - `run.results()` / `SarifResultIter` -> `SarifResult`（`rule_id`, `level`, `message`, `artifact_uri`, `start_line`, `start_col`, `end_line`）
- **单遍汇总**：`summarizeSarif(data): SarifStats`（`run_count`, `total_results`, `error_count`, `warning_count`, `note_count`, `none_count`, `hasErrors(): bool`）

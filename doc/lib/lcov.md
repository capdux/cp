# `cap:lcov` — 纯 CP LCOV `.info` 代码覆盖率零分配流式解析库

## 概述

`cap:lcov` 提供纯 CP、零外部依赖、零堆分配的 LCOV tracefile（`.info`）逐行流式解析与文件/全局覆盖率统计库：
- **完整 LCOV 语法支持**：`TN`、`SF`、`FN`、`FNDA`、`FNF`、`FNH`、`DA`（含可选 MD5 checksum）、`LF`、`LH`、`BRDA`（支持 `"-"` 未执行分支）、`BRF`、`BRH` 与 `end_of_record`；
- **多平台换行与特殊字符兼容**：无缝兼容 `\n` / `\r\n`，支持函数名包含逗号及 C++/泛型符号；
- **智能汇总回退**：`LcovFileIter` 缺失 `LF`/`LH`/`BRF`/`BRH`/`FNF`/`FNH` 时自动回退由明细行实算，`summarizeLcov` 单遍汇总全局覆盖率。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `LcovRecordKind` | 14 种 LCOV 记录类型枚举 |
| `LcovLineHit`, `LcovBranchHit`, `LcovFnDef`, `LcovFnHit`, `LcovRecord` | 零拷贝明细记录结构 |
| `LcovRecordIter` | 逐行流式记录迭代器（`init(text)`、`next(*_): LcovRecord?`） |
| `LcovFileSummary`, `LcovFileIter` | 文件级覆盖率聚合（含 `lineCoveragePct()`、`branchCoveragePct()`、`fnCoveragePct()`） |
| `LcovTotals`, `summarizeLcov(text)` | 单遍汇总整份 `.info` 报告的文件数与总覆盖率 |

## 示例

```cp
import { summarizeLcov } from "cap:lcov";

fn checkCoverage(info_text: []const u8): bool {
    const totals = summarizeLcov(info_text);
    return totals.file_count > 0 && totals.lineCoveragePct() >= 80.0;
}
```

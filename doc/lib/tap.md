# `cap:tap` — 零分配 TAP v12/v13/v14 测试协议解析与汇总标准库

`cap:tap` 是纯 CP、零堆分配的 Test Anything Protocol（TAP v12 / v13 / v14）流式解析与汇总库，支持版本头、前置/尾置计划（含 `1..0 # SKIP` 全跳过计划）、测试点、大小写不敏感 `# SKIP` / `# TODO` 指令、`\#` 转义井号识别、缩进 YAML 诊断块聚合、`pragma` 与 `Bail out!` 提前终止。

---

## 快速导入

```typescript
import {
    TapEventKind,
    TapDirective,
    TapEvent,
    TapSummary,
    TapIter,
    summarizeTap,
} from "cap:tap";
```

---

## 核心 API

- **枚举**：
  - `TapEventKind: u8`（`version`, `plan`, `test_point`, `bail_out`, `yaml_block`, `comment`, `pragma`）
  - `TapDirective: u8`（`none`, `skip`, `todo`）
- **事件迭代器**：`TapIter.init(data)` / `next(*_): TapEvent?`
- **汇总统计**：`summarizeTap(data): TapSummary`，含 `version`, `planned`, `has_plan`, `total`, `passed`, `failed`, `skipped`, `todo_passed`, `todo_failed`, `bailed_out` 与只读判定 `allPassed(): bool`。

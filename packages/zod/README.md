[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# zod

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:zod` ([https://github.com/colinhacks/zod](https://github.com/colinhacks/zod))（原库名 `zod` → CP 包名 `zod`）

零堆分配的 CP 原生类型安全数据校验框架，Clean-Room MIT 实现对标 npm Top 25 顶流包 `zod` v3（1.5 亿+/周下载）。

跨包零重复复用 `kind-of`（运行时字面量类型判别）与 `url`（WHATWG 标准 URL 解析校验），单遍扫描顶层 JSON 对象并一次性收集全部字段错误。

## 核心 API

### 1. 规则类型 (`ZRuleKind`)
- `Z_STRING (1)` / `Z_NUMBER (2)` / `Z_INT (3)` / `Z_BOOL (4)`
- `Z_EMAIL (5)` / `Z_URL (6)` / `Z_UUID (7)` / `Z_ENUM (8)`
- `Z_ARRAY (9)` / `Z_OBJECT (10)`

### 2. Schema 结构体与解析
- `ZFieldRule`：声明字段名、类型、是否必填、数值范围 (`min_val`/`max_val`)、长度范围 (`min_len`/`max_len`)、逗号分隔枚举集 (`enum_csv`)
- `ZObjectSchema`：最多支持 24 条字段规则，提供 `new()`、`field(*_, rule)`、`safeParseJson(json)`
- `ZParseResult` / `ZIssue`：零堆分配错误列表（最多收集 16 条精确字段与错误码）

### 3. 独立快捷校验器
- `zString(raw, min_len, max_len)`
- `zNumber(raw, min_v, max_v)`
- `zEmail(s)`
- `zUrl(s)`（直接委托 `WhatwgUrl.parse`）
- `zUuid(s)`
- `zEnum(s, csv)`

## 依赖复用

- `kind-of`: `1.0.0`
- `url`: `1.0.0`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`colinhacks/zod`](https://github.com/colinhacks/zod)。

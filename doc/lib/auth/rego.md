# `cap:rego` — 纯 CP Open Policy Agent (OPA) Rego 策略零分配词法扫描与校验库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:auth`，导入请使用 `import { ... } from "cap:auth"`。

## 概述

`cap:rego` 提供纯 CP、零外部依赖、零堆分配的 Open Policy Agent (OPA) Rego 策略语言词法扫描器、包名提取器、`import` 路径迭代器、顶层规则名迭代器与括号深度结构校验器，适用于策略引擎预检、依赖分析与安全护栏场景。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `REGO_MAX_DEPTH` | 定界符最大允许嵌套深度常量（`64`） |
| `RegoTokenKind` | 词法记号枚举：`ident`, `string`, `raw_string`, `number`, `bool_lit`, `null_lit`, `assign` (`:=`), `lbrace`, `rbrace`, `lbracket`, `rbracket`, `lparen`, `rparen`, `colon`, `comma`, `dot`, `semicolon`, `punct`, `invalid` |
| `RegoToken` | 零拷贝词法记号：`kind: RegoTokenKind`, `lexeme: []const u8` |
| `RegoIter` | 流式词法扫描器：`init(input)`, `next(*_): RegoToken?`；自动跳过 `#` 行注释，识别双引号转义字符串、反引号原始字符串（支持跨行）、`:=`、数字、`true`/`false`/`null` 与标识符 |
| `extractRegoPackage(input)` | 提取顶层 `package authz.api.v1` 完整点分包名切片；缺失或畸形返回 `null` |
| `RegoImportIter` | `import` 路径流式迭代器：`init(input)`, `next(*_): ([]const u8)?`；提取所有顶层 `import` 目标路径（自动剥离 `as alias`，忽略注释/字符串/块内文本） |
| `RegoRuleIter` | 顶层规则名流式迭代器：`init(input)`, `next(*_): ([]const u8)?`；兼容 `default allow := false`、`allow if { ... }`、`allow := true if { ... }`、`roles contains r if { ... }`、集合/对象/函数规则，自动跳过 `package`/`import`/`else` |
| `validateRegoDelimiters(input)` | 字符串/原始字符串/注释感知校验 `{}`、`[]`、`()` 平衡性、词法闭合性与 64 层嵌套深度上限 |

## 示例

```cp
import { extractRegoPackage, RegoImportIter, RegoRuleIter, validateRegoDelimiters } from "cap:auth";

fn inspectPolicy(src: []const u8): bool {
    if (!validateRegoDelimiters(src)) return false;
    if (pkg := extractRegoPackage(src)) {
        let imports = RegoImportIter.init(src);
        let rules = RegoRuleIter.init(src);
        return pkg.len > 0 && (imports.next() != null || rules.next() != null);
    }
    return false;
}
```

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/strnum

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:strnum` ([https://github.com/NaturalIntelligence/strnum](https://github.com/NaturalIntelligence/strnum))（原库名 `strnum` → CP 包名 `@cap/strnum`）

零堆分配的 CP 原生可配置字符串转数字解析器，Clean-Room MIT 实现对标 npm 顶流解析库 `strnum`（1.02 亿+/周下载）。

精确控制前导零（`leading_zeros`）、十六进制（`hex`）与科学计数法（`e_notation`）解析策略，对空白、非数字字符与溢出输入安全回退（`is_number = false`）。

## 核心 API

- `StrNumOptions` — 解析配置项（`hex: bool`, `leading_zeros: bool`, `e_notation: bool`）
- `StrNumResult` — 解析结果（`is_number: bool`, `value: f64`, `is_int: bool`, `int_value: i64`）
- `parseStrNum(input: string, opts: StrNumOptions): StrNumResult` — 按配置解析字符串为数值

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`NaturalIntelligence/strnum`](https://github.com/NaturalIntelligence/strnum)。

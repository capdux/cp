[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/has-flag

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:has-flag` ([https://github.com/sindresorhus/has-flag](https://github.com/sindresorhus/has-flag))

零堆分配的 CP 原生 CLI 标志检测库，Clean-Room MIT 实现对标 npm 顶流包 `has-flag`（2.5亿+/周下载）。

## 特性

- 自动补齐单字符 `-f` 与多字符 `--flag` 前缀，同时兼容显式传入 `-f` / `--flag`
- 严格遵循 POSIX `--` 选项终止符规则（`--` 之后的参数不再匹配）
- 支持 `--flag=value` 精确匹配与值提取（`getFlagValue`）
- 支持 `--flag` / `--no-flag` 后置覆盖布尔开关（`isFlagEnabled`）

## 使用示例

```cp
import { hasFlag, getFlagValue, isFlagEnabled } from "@cap/has-flag";

let args = ["--color", "--level=3", "--", "--ignored"];
let c = hasFlag("color", args);           // true
let l = hasFlag("level=3", args);         // true
let ig = hasFlag("ignored", args);        // false (在 `--` 之后)
let en = isFlagEnabled("color", args, false); // true
```

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`sindresorhus/has-flag`](https://github.com/sindresorhus/has-flag)。

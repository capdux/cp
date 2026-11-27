[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# toidentifier

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:toidentifier` ([https://github.com/dougwilson/toidentifier](https://github.com/dougwilson/toidentifier))

零堆分配的 CP 原生标识符转换库，Clean-Room MIT 实现对标 npm 顶流包 `toidentifier`（9000万+/周下载，`http-errors` 核心依赖）。

## 特性

- `toIdentifier`: 将自然语言词组转为大驼峰标识符（如 `"Bad Request"` → `"BadRequest"`、`"I'm a teapot"` → `"ImATeapot"`）
- `toCamelIdentifier`: 转为小驼峰标识符（`"badRequest"`）
- `toConstIdentifier`: 转为全大写下划线常量标识符（`"BAD_REQUEST"`）
- `isValidIdentifier`: 校验合法 ASCII 标识符

## 使用示例

```cp
import { toIdentifier, isValidIdentifier } from "toidentifier";

let buf: [64]u8 = undefined;
let n = toIdentifier("I'm a teapot", buf[0..64]); // "ImATeapot" (len 9)
```

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`dougwilson/toidentifier`](https://github.com/dougwilson/toidentifier)。

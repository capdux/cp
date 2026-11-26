<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:esutils` ([https://github.com/estools/esutils](https://github.com/estools/esutils)) | 许可：`BSD-2-Clause`

# `@cap/esutils`

零堆分配纯 CP ECMAScript 词法字符、关键字、保留字与标识符校验工具集，对标 npm **#201** `esutils`（真实周下载 **2.15 亿/周**，ESLint/escodegen 底层核心依赖）。

## 核心 API

- 字符类：`isDecimalDigit`, `isHexDigit`, `isOctalDigit`, `isWhiteSpace`, `isLineTerminator`
- 标识符与保留字：`isIdentifierStartES6`, `isIdentifierPartES6`, `isKeywordES6`, `isReservedWordES6`, `isRestrictedWord`, `isIdentifierNameES6`, `isIdentifierES6`

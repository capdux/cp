# `cap:js` — JavaScript 词法标识符与运算符元信息标准库

> 参考来源：/data/dev/bun/src/js_parser/ast/Op.cp, /data/dev/bun/src/js_parser/lexer/ | 吸收日期：2026-11-22

## 概述

`cap:js` 吸收自 Bun JS/TS 词法分析器与 AST 运算符元信息表（`src/js_parser/ast/Op.cp` 与 `src/js_parser/lexer/`），统一聚合 ECMAScript 2024 / TypeScript 关键字分类、保留字判定、ASCII 标识符与私有字段（`#field`）校验、对象属性引号判定、JS 字符串转义，以及完整运算符优先级、结合性与赋值目标语义查询。

## 上游语法契约与分类体系

- **四级关键字分类（`KeywordKind`）**：
  - `reserved`：无条件 ECMAScript 保留关键字（如 `class`、`return`、`function`、`await`、`import`、`export` 等）；
  - `strict_reserved`：严格模式保留字（如 `implements`、`interface`、`package`、`private`、`protected`、`public`、`static`）；
  - `contextual`：上下文关键字（如 `async`、`from`、`of`、`type`、`declare` 等）；
  - `none`：普通标识符（长度 `< 2` 或 `> 10` 直接 $O(1)$ 短路返回 `.none`）。
- **运算符元信息表（吸收自 Bun `Op.cp`）**：
  - 完整覆盖前缀一元（`un_pos`/`un_neg`/`un_cpl`/`un_not`/`un_void`/`un_typeof`/`un_delete`）、前后缀自增自减（`un_pre_dec`/`un_pre_inc`/`un_post_dec`/`un_post_inc`）、左结合二元（`bin_add`..`bin_bitwise_xor`）、非结合逗号（`bin_comma`）以及右结合复合赋值（`bin_assign`、`bin_add_assign` 等）；
  - 通过 `AssignTarget`（`none` / `replace` / `update`）精准区分普通表达式、纯替换赋值（`=`）与读-改-写复合赋值（`+=`、`++`），供 AST 压缩器与静态分析器判断左值读写语义。

## 目录结构

- `lib/js/ident.cp`：JS/TS 关键字分类、保留字判定、ASCII 标识符与私有字段校验、属性引号判定及字符串转义
- `lib/js/op.cp`：JS AST 运算符枚举、优先级层级、赋值目标类型及元信息表
- `lib/js/useDirective.cp`：JS/RSC 顶层指令序言（`"use client"` / `"use server"` / `"use strict"`）解析器与边界切换判定（吸收自 `/data/dev/bun/src/js_parser/ast/UseDirective.cp`）
- `lib/js/index.cp`：`cap:js` 统一聚合导出入口

## 核心 API

### 标识符与关键字 (`ident.cp`)

- `KeywordKind`：`none` / `reserved` / `strict_reserved` / `contextual`
- `classifyKeyword(s: []const u8): KeywordKind`
- `isReservedWord(s: []const u8): bool`
- `isValidAsciiIdent(s: []const u8): bool`
- `isValidPrivateIdent(s: []const u8): bool`
- `needsPropertyQuoting(s: []const u8): bool` — 判断对象字面量键名是否需要加引号（如含连字符或以数字开头）
- `escapeJsString(alloc, s, quote)` — 按指定引号（`'` 或 `"`）转义 JS 字符串（含换行、反斜杠、控制字符）

### 运算符元信息 (`op.cp`)

- `AssignTarget`：`none` / `replace` / `update`
- `Code`：完整 JS/TS 运算符枚举（如 `Code.bin_add`、`Code.bin_nullish_coalescing`、`Code.bin_assign`）
- `Level`：运算符优先级层级枚举
- `OpInfo`：包含运算符文本、优先级 `Level`、结合性与 `AssignTarget`
- `opInfo(code: Code): OpInfo` — $O(1)$ 查表返回运算符元信息

## 边界注意事项

1. **ASCII 标识符范围**：`isValidAsciiIdent` 面向高频 ASCII 快速路径（首字符 `[a-zA-Z_$]`，后续 `[a-zA-Z0-9_$]`），不包含完整 Unicode ID_Start/ID_Continue 表。
2. **私有标识符格式**：`isValidPrivateIdent` 要求以 `#` 开头且后续为合法标识符（如 `#secret`），单独 `"#"` 返回 `false`。

## 使用示例

```cp
import { isReservedWord, isValidAsciiIdent, Code, opInfo } from "cap:js";

fn demo() {
    const reserved = isReservedWord("class");     // true
    const valid = isValidAsciiIdent("foo_bar");   // true
    const info = opInfo(Code.bin_add);
    * = reserved;
    * = valid;
    * = info;
}
```

### 顶层指令序言与 RSC 边界判定 (`useDirective.cp`)

吸收自 `/data/dev/bun/src/js_parser/ast/UseDirective.cp`（参考 `/data/dev/bun/docs/bundler/index.mdx` 与 `/data/dev/bun/docs/snippets/cli/build.mdx` 中 `--banner '"use client";'` 打包器实战指南）：

- **导出符号**：`UseDirectiveBoundering`（`Client`/`Server`）、`UseDirectiveFlags`（`record`/`hasAnyBoundary`）、`UseDirective`（`None`/`Client`/`Server`/`Strict`；方法 `isBoundary`/`boundering`/`asLiteral`/`asBanner`/`parse`）、`scanUseDirectives(contents)`；
- **打包器实战最佳实践**：
  - React Server Components 模块可通过文件顶部 `"use client";` 或 `"use server";` 声明执行边界；当打包输出需要保留顶层指令时，可配合 `UseDirective.Client.asBanner()`（`'"use client";'`）注入 `--banner`；
  - `UseDirective.parse` 自动跳过前导 UTF-8 BOM、空白、分号与 `//` / `/* ... */` 注释，区分非指令代码（`.None`）与未知引号指令（`null`）；`scanUseDirectives` 可扫描连续指令序言（如 `"use strict";\n"use client";`）。

```cp
import { UseDirective, scanUseDirectives } from "cap:js";

const dir = UseDirective.parse("\"use client\";\nexport default function Btn() {}");
const crosses = UseDirective.Server.isBoundary(.Client); // true
const flags = scanUseDirectives("\"use strict\";\n'use client';");
* = dir; * = crosses; * = flags;
```

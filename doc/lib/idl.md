# `cap:idl` — 纯 CP 零分配 W3C WebIDL 词法扫描与顶层声明提取库

## 概述

`cap:idl` 提供纯 CP、零外部依赖、零堆分配的 W3C WebIDL 词法扫描器（`WebIdlIter`）、顶层声明提取迭代器（`WebIdlDeclIter`）与字符串/注释感知括号平衡校验器（`validateWebIdlBraces`），广泛适用于 Bun / CapSurf Web API 绑定生成与静态分析。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `WebIdlDeclKind` | 顶层声明类型枚举：`interface_decl`, `interface_mixin`, `dictionary_decl`, `enum_decl`, `namespace_decl`, `typedef_decl`, `callback_decl`, `includes_decl` |
| `WebIdlDecl` | 顶层声明结构体：`kind`, `is_partial`, `name`, `inherits: ([]const u8)?`, `ext_attrs: ([]const u8)?`（`[Exposed=Window, ...]` 切片）, `body: ([]const u8)?`（`{ ... }` 内部或 `typedef`/`callback` 表达式切片） |
| `WebIdlTokenKind` | 词法 Token 枚举：`ident`, `string`, `number`, `punct`, `invalid` |
| `WebIdlToken` | 词法 Token：`kind`, `lexeme: []const u8`（零拷贝指向原始输入） |
| `WebIdlIter` | 词法扫描迭代器：`init(input)`, `next(*_): WebIdlToken?`（跳过 `//` 与 `/* */` 注释，识别双引号字符串、整/十六进制/浮点数字、标识符、`...` 及标点） |
| `WebIdlDeclIter` | 顶层声明提取器：`init(input)`, `next(*_): WebIdlDecl?`（支持可选 `[ExtAttr]` 前缀与 `partial` 修饰符，提取全部 8 类 WebIDL 顶层声明） |
| `validateWebIdlBraces(input)` | 字符串/注释感知校验 `{}` / `()` / `[]` / `<>` 平衡与 64 层最大嵌套深度上限 |

## 示例

```cp
import {
    WebIdlDeclIter,
    WebIdlDeclKind,
    validateWebIdlBraces,
} from "cap:idl";

fn inspectWebIdl(src: []const u8): usize {
    if (!validateWebIdlBraces(src)) {
        return 0;
    }
    let count: usize = 0;
    let it = WebIdlDeclIter.init(src);
    while (decl := it.next()) {
        if (decl.kind == WebIdlDeclKind.interface_decl) {
            count += 1;
        }
    }
    return count;
}
```

# `cap:sexp` — 纯 CP 零分配 S-Expression 词法扫描、平衡校验与格式化器

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 概述

`cap:sexp` 提供纯 CP、零外部依赖、零堆分配的 S-Expression（WASM WAT / Lisp / AST 调试输出）词法扫描器、括号平衡与最大嵌套深度校验器，以及去注释紧凑规范格式化器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `SexpTokenKind` | 词法单元类型枚举：`lparen`、`rparen`、`atom`、`string`、`invalid` |
| `SexpToken` | 零拷贝词法单元：`kind: SexpTokenKind`, `lexeme: []const u8` |
| `SexpIter` | 流式词法扫描器：`init(input)`、`next(*_): SexpToken?`（跳过空白与 `;` 行注释，支持双引号 `\` 转义，未闭合字符串返回 `invalid`） |
| `validateSexp(input)` | 校验括号嵌套平衡、无未闭合字符串、无多余 `)` 且至少含 1 个有效 token |
| `maxSexpDepth(input)` | 计算最大括号嵌套深度，语法非法或空输入返回 `null` |
| `formatSexpCompact(input, out)` | 去除注释与多余空白输出紧凑规范 S-Expression，非法输入或缓冲不足返回 `null` |

## 示例

```cp
import { SexpIter, SexpTokenKind, validateSexp, maxSexpDepth, formatSexpCompact } from "cap:compiler";

fn example(): bool {
    const src = "(module ; wat\n  (func $add (param i32 i32) (result i32)))";
    if (!validateSexp(src)) return false;
    let buf: [128]u8 = undefined;
    if (compact := formatSexpCompact(src, buf[0..])) {
        return compact.len > 0 && (maxSexpDepth(src) ?? 0) == 3;
    }
    return false;
}
```

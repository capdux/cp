# `cap:js` — JavaScript 词法标识符与运算符元信息标准库

`cap:js` 聚合了 JavaScript 词法标识符校验、关键字分类、字符串转义以及运算符优先级与结合性元信息查询能力。

## 目录结构

- `lib/js/ident.cp`：JS/TS 关键字分类、保留字判定、ASCII 标识符与私有字段校验、属性引号判定及字符串转义
- `lib/js/op.cp`：JS AST 运算符枚举、优先级层级、赋值目标类型及元信息表
- `lib/js/index.cp`：`cap:js` 统一聚合导出入口

## 核心 API

### 标识符与关键字 (`ident.cp`)
- `KeywordKind`：关键字分类枚举
- `classifyKeyword(s: []const u8): KeywordKind`
- `isReservedWord(s: []const u8): bool`
- `isValidAsciiIdent(s: []const u8): bool`
- `isValidPrivateIdent(s: []const u8): bool`
- `needsPropertyQuoting(s: []const u8): bool`
- `escapeJsString(alloc, s, quote)`

### 运算符元信息 (`op.cp`)
- `AssignTarget`、`Code`、`Level`、`OpInfo`
- `opInfo(code: Code): OpInfo`

## 使用示例

```cp
import { isReservedWord, isValidAsciiIdent, Code, opInfo } from "cap:js";

fn demo() {
    const reserved = isReservedWord("class");
    const valid = isValidAsciiIdent("foo_bar");
    const info = opInfo(Code.add);
    * = reserved;
    * = valid;
    * = info;
}
```

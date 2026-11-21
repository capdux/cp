# `cap:js_op` — JS/TS AST 运算符编码、优先级与结合性元数据表

> 吸收自 `/data/dev/bun/src/js_parser/ast/Op.cp`，提供 ECMAScript / TypeScript 全部 53 个一元、二元与赋值运算符的零分配元数据查询、23 级优先级比较、左右结合性及赋值目标判定。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `AssignTarget` | 赋值目标种类：`none`、`replace`（`=`）、`update`（`+=`/`++` 等） |
| `Code` | 53 项完整 JS/TS 运算符枚举，提供 `unaryAssignTarget()`、`binaryAssignTarget()`、`isLeftAssociative()`、`isRightAssociative()`、`isPrefix()`、`isPostfix()`、`isUnary()`、`isBinary()`、`info()` |
| `Level` | 23 级运算符优先级枚举（`lowest`..`member`），提供 `lt`/`gt`/`lte`/`gte`/`eql` |
| `OpInfo` / `opInfo(code)` | 返回 `{ text: string, level: Level, is_keyword: bool }` |

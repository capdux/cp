# cap:spdx_license

> 源码吸收自 `/data/dev/bun/src/install/PackageManager.zig`（SPDX 2.3 规范），提供纯 CP、零外部依赖、零堆分配的 SPDX 开源许可证表达式词法/语法校验器与许可证 ID 迭代器。

## 导入方式

```cp
import {
    SpdxTokenKind,
    SpdxToken,
    SpdxLexer,
    isValidSpdxExpression,
    SpdxIdIter,
    isCommonPermissive,
} from "cap:spdx_license";
```

## 核心类型与 API

- `SpdxTokenKind`：`ident` / `and_op` / `or_op` / `with_op` / `lparen` / `rparen` / `invalid`。
- `SpdxToken`：`kind: SpdxTokenKind`、`text: []const u8`。
- `SpdxLexer`：`init(expr)`、`next(*_): SpdxToken?`、`peek(): SpdxToken?`（区分严格大写 `AND`/`OR`/`WITH`，支持 `+` 后缀、`LicenseRef-...` 与 `DocumentRef-...:LicenseRef-...`）。
- `isValidSpdxExpression(expr: []const u8): bool`：递归下降校验完整 SPDX 2.3 语法（括号平衡、`WITH` 左右约束、禁止空括号/连续运算符/尾随运算符）。
- `SpdxIdIter`：`init(expr)` / `next(*_): ([]const u8)?`（提取表达式中所有许可证 ID，跳过运算符与异常 ID）。
- `isCommonPermissive(id: []const u8): bool`：识别 `MIT`/`ISC`/`BSD-2-Clause`/`BSD-3-Clause`/`Apache-2.0`/`0BSD`/`Unlicense`/`CC0-1.0`/`Zlib`/`BlueOak-1.0.0`。

## 使用示例

```cp
import { isValidSpdxExpression, SpdxIdIter, isCommonPermissive } from "cap:spdx_license";

fn example() {
    const expr = "(MIT OR Apache-2.0 WITH LLVM-exception)";
    if (isValidSpdxExpression(expr)) {
        let it = SpdxIdIter.init(expr);
        while (it.next() != null) {}
    }
    * = isCommonPermissive("MIT"); // true
}
```

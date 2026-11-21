# cap:js_ident

> 源码吸收自 `/data/dev/bun/src/js_parser/lexer/`，提供纯 CP、零外部依赖、零堆分配的 ECMAScript / TypeScript 标识符校验、关键字分类与 JS 字符串安全转义工具库。

## 导入方式

```cp
import {
    KeywordKind,
    classifyKeyword,
    isReservedWord,
    isValidAsciiIdent,
    isValidPrivateIdent,
    needsPropertyQuoting,
    escapeJsString,
} from "cap:js_ident";
```

## 核心类型与 API

### `KeywordKind`

- `none`：普通标识符
- `reserved`：ES2024 全部 36 个保留字（`break`..`with`）
- `strict_reserved`：10 个严格模式保留字（`implements`/`interface`/`let`/`package`/`private`/`protected`/`public`/`static`/`yield`/`await`）
- `contextual`：10 个上下文关键字（`as`/`async`/`from`/`get`/`meta`/`of`/`set`/`target`/`type`/`satisfies`）

### 核心函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `classifyKeyword` | `fn classifyKeyword(word: []const u8): KeywordKind` | 精确分类 ES2024/TS 关键字 |
| `isReservedWord` | `fn isReservedWord(word: []const u8, strict_mode: bool): bool` | 判断在普通/严格模式下是否为保留字 |
| `isValidAsciiIdent` | `fn isValidAsciiIdent(s: []const u8): bool` | 校验 `[A-Za-z_$][A-Za-z0-9_$]*` |
| `isValidPrivateIdent` | `fn isValidPrivateIdent(s: []const u8): bool` | 校验 `#` + 合法标识符 |
| `needsPropertyQuoting` | `fn needsPropertyQuoting(key: []const u8, strict_mode: bool): bool` | 判断 JS 对象属性键是否需要加引号 |
| `escapeJsString` | `fn escapeJsString(raw: []const u8, quote: u8, out: []u8): ([]const u8)?` | 转义控制字符、`\n`/`\r`/`\t`/`\\`/引号及 `\u2028`/`\u2029` |

## 使用示例

```cp
import { needsPropertyQuoting, escapeJsString } from "cap:js_ident";

fn example() {
    * = needsPropertyQuoting("content-type", true); // true
    * = needsPropertyQuoting("userId", true);       // false

    let buf: [64]u8 = undefined;
    * = escapeJsString("hi\n\"cp\"", '"', &buf);    // "\"hi\\n\\\"cp\\\"\""
}
```

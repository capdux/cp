# `cap:dotenv_escape` — `.env` 键名合法性校验与安全序列化库

> 来源：吸收自 `/data/dev/bun/src/env_loader.zig`（`.env` Key Validator & Safe Value Serializer）

`cap:dotenv_escape` 提供纯 CP、零外部依赖、零堆分配的 `.env` 环境变量键名校验、引号必要性检测、防 `$VAR` 误展开的双引号安全序列化以及反转义往返能力。

## 核心函数

```cp
pub fn isValidEnvKey(key: []const u8): bool
pub fn needsDotenvQuotes(val: []const u8): bool
pub fn formatDotenvValue(val: []const u8, out: []u8): ([]const u8)?
pub fn formatDotenvLine(key: []const u8, val: []const u8, out: []u8): ([]const u8)?
pub fn unescapeDotenvDouble(escaped: []const u8, out: []u8): ([]const u8)?
```

- `isValidEnvKey`：严格校验 POSIX 环境变量键名 `[A-Za-z_][A-Za-z0-9_]*`。
- `needsDotenvQuotes`：当值为空串或包含空白、控制字符、`#`、`"`、`'`、`\`、`$`、`` ` `` 时返回 `true`。
- `formatDotenvValue`：安全值原样输出；否则双引号包裹并转义 `\"`、`\\`、`\n`、`\r`、`\t`、`\$`。
- `formatDotenvLine`：校验 `key` 合法后输出 `KEY=VALUE`。
- `unescapeDotenvDouble`：还原双引号转义序列，兼容含外层 `"` 或纯内部内容。

## 使用示例

```cp
import { formatDotenvLine, unescapeDotenvDouble } from "cap:dotenv_escape";

let buf: [128]u8 = [0; 128];
const line = formatDotenvLine("API_SECRET", "p@ss$word #1", buf[0..]).?;
// line == "API_SECRET=\"p@ss\\$word #1\""
```

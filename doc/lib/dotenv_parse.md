# `cap:dotenv_parse` — 零堆分配 `.env` 解析器与变量展开器

> 来源：吸收自 `/data/dev/bun/src/bun_core/env.cp`

`cap:dotenv_parse` 提供纯 CP、零外部依赖、零堆分配的 `.env` 配置文件迭代器、双引号解转义及 `$VAR` / `${VAR}` / `${VAR:-default}` 变量展开能力。

## 核心类型

### `DotEnvQuote` & `DotEnvEntry`

```cp
pub enum DotEnvQuote: u8 {
    unquoted = 0,
    single = 1,
    double = 2,
    backtick = 3,

    fn asStr(): string
}

pub struct DotEnvEntry {
    key: []const u8 = "",
    raw_value: []const u8 = "",
    quote: DotEnvQuote = .unquoted,
}
```

### `DotEnvIterator`

```cp
pub struct DotEnvIterator {
    src: []const u8,
    pos: usize = 0,

    static fn init(src: []const u8): DotEnvIterator
    fn next(*_): DotEnvEntry?
}
```

- 自动剥离 UTF-8 BOM、跳过空行与 `#` 注释行、跳过缺失 `=` 的畸形行。
- 自动剥离可选前导 `export ` 关键字（不影响以 `export_` 开头的合法键名）。
- 无引号值自动剥离空白后的行尾 `# comment` 与首尾空白，保留值内部的 `#`（如 `#ff00ff`）。
- 完整支持单引号 `'...'`、双引号 `"..."`（含 `\"` 转义与多行）和反引号 `` `...` ``。

## 核心函数

### `unescapeDoubleQuoted`

```cp
pub fn unescapeDoubleQuoted(raw: []const u8, out: []u8): ([]const u8)?
```

- 解转义双引号 `.env` 值中的 `\n`、`\r`、`\t`、`\"`、`\\`、`\$`。

### `expandVars`

```cp
pub type EnvLookupFn = fn ([]const u8): ([]const u8)?;

pub fn expandVars(input: []const u8, lookup_fn: EnvLookupFn, out: []u8): ([]const u8)?
```

- 展开 `$VAR`、`${VAR}` 与 `${VAR:-default}`（变量未定义或为空时回退至 `default`），支持 `\$` 字面量逃逸。

## 使用示例

```cp
import { DotEnvIterator, unescapeDoubleQuoted, expandVars } from "cap:dotenv_parse";

let iter = DotEnvIterator.init("export PORT=8080 # server\nHOST=\"localhost\"\n");
while (entry := iter.next()) {
    // entry.key, entry.raw_value, entry.quote
}
```

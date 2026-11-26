# @cap/yargs-parser

Zero-allocation CLI argument parser for CP — clean-room MIT parity with npm `yargs-parser`. Reuses `@cap/camelcase` and `@cap/decamelize` for automatic camelCase & kebab-case option key lookup.

纯 CP 零堆分配命令行参数解析器（对标 npm `yargs-parser`），完整支持 `--key=val` / `--key val` / `--flag` / `--no-flag`、短选项与 `-abc` 组合标志、负数数值防误判、`--` 终止符原样透传、数字/布尔自动类型识别，以及基于 `@cap/camelcase` + `@cap/decamelize` 的双命名风格键查询。

## Install / 安装

```bash
cap add @cap/yargs-parser
```

## Dependencies / 跨包复用

- `@cap/camelcase`: 自动将 `--max-retries` 键名驼峰化为 `maxRetries`
- `@cap/decamelize`: 自动还原 `--maxRetries` 为规范连字符名 `max-retries`

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `ARG_BOOL` / `ARG_INT` / `ARG_STRING` | `const u8` (`1`, `2`, `3`) | Coerced argument value kinds |
| `ParsedEntry` | `struct` | Fixed-capacity option entry (`key`, `camel_key`, `kebab_key`, typed values) |
| `ParsedArgs` | `struct` | Container with `.has()`, `.getBool()`, `.getInt()`, `.getString()`, `.positionalCount()`, `.positionalAt()`, `.restCount()`, `.restAt()` |
| `parseArgs` | `(argv: []const string): ParsedArgs` | Parse CLI `argv` slice with zero heap allocation |

## Usage / 使用示例

```cp
import { parseArgs } from "@cap/yargs-parser";

const argv: [4]string = ["--max-retries=5", "-abc", "-p", "8080"];
const parsed = parseArgs(argv[0..4]);
const retries = parsed.getInt("maxRetries", 0); // 5
const port = parsed.getInt("p", 3000);          // 8080
```

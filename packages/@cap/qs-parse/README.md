# @cap/qs-parse

Zero-allocation Express/Node nested query string parser and serializer for CP — clean-room MIT parity with npm `qs` v6. Composes `@cap/serde-qs` and `@cap/percent-encoding` with zero duplication.

纯 CP 零堆分配 Express/Node 嵌套查询串解析与格式化引擎（对标 npm `qs` v6），跨包复用 `@cap/serde-qs`（嵌套括号路径解析）与 `@cap/percent-encoding`（WHATWG/RFC 百分号编解码）。

## Install / 安装

```bash
cap add @cap/qs-parse
```

## Dependencies / 跨包复用

- `@cap/serde-qs`: 复用 `QsPair`、`QsPath` 与 `parseKeyPath` 进行零堆分配嵌套括号路径拆分
- `@cap/percent-encoding`: 复用 `componentSet`、`percentEncode` 与 `percentDecodePlus` 进行 RFC/WHATWG 百分号编解码

## API

### Array Format Constants / 数组格式常量

| Constant | Value | Output Example |
| --- | --- | --- |
| `QS_ARRAY_INDICES` | `0` | `a[0]=x&a[1]=y` |
| `QS_ARRAY_BRACKETS` | `1` | `a[]=x&a[]=y` |
| `QS_ARRAY_REPEAT` | `2` | `a=x&a=y` |
| `QS_ARRAY_COMMA` | `3` | `a=x,y` |

### Core Types & Functions / 核心结构体与函数

| Symbol | Signature | Description |
| --- | --- | --- |
| `QsOptions` | `struct { delimiter: u8, allow_dots: bool, max_depth: usize, array_format: u8, add_query_prefix: bool }` + `.defaultOpts()` | Parser & serializer configuration |
| `parseQuery` | `(query: string, opts: QsOptions, out_pairs: []QsPair): usize` | Parse query string with custom delimiter, leading `?` skip, `allow_dots`, and `max_depth` guard |
| `parseKeyPathWithDots` | `(key: string, allow_dots: bool): QsPath` | Split bracket (`a[b]`) and optional dot (`a.b`) key paths into up to 8 segments |
| `stringifyPairs` | `(pairs: []const QsPair, opts: QsOptions, out: []u8): usize` | Serialize `QsPair` slice with optional `?` prefix, custom delimiter, and percent-encoding |
| `stringifyArray` | `(key: string, values: []const string, array_format: u8, out: []u8): usize` | Serialize array values in `indices`, `brackets`, `repeat`, or `comma` format |
| `decodeQueryValue` | `(raw: string, out: []u8): usize` | Decode `+` and `%XX` query values via `@cap/percent-encoding` |

## Usage / 使用示例

```cp
import {
    QS_ARRAY_BRACKETS,
    QsOptions,
    QsPair,
    parseQuery,
    stringifyPairs,
    stringifyArray,
    decodeQueryValue,
} from "@cap/qs-parse";

let pairs: [8]QsPair = undefined;
const count = parseQuery("?a[b]=hello+world&c=1", QsOptions.defaultOpts(), pairs[0..8]);

let out: [128]u8 = undefined;
const vals: [2]string = ["foo", "bar"];
const n = stringifyArray("tags", vals[0..2], QS_ARRAY_BRACKETS, out[0..128]);
// "tags[]=foo&tags[]=bar"
```

# `cap:logfmt` — 结构化 `logfmt` 日志解析器与原子编码器

> 参考来源：Rust [`logfmt`](https://crates.io/crates/logfmt) / Go [`go-logfmt`](https://github.com/go-logfmt/logfmt) & `/data/dev/bun/src/logger.zig` | 深度优化日期：2026-11-26

## 概述

`cap:logfmt` 提供纯 CP、零堆分配、防日志注入的结构化 `logfmt`（`key=value`）解析与编码工具集，支持：
- 零拷贝流式迭代 `LogfmtIter`（支持引号转义、裸布尔键、未闭合引号错误检测）；
- 字段类型化解析 `asInt` / `asUint` / `asFloat` / `asBool` 与便捷按键查询；
- 原子级结构化日志构造器 `LogfmtEncoder`（缓冲不足时自动回滚，绝不写出残缺半截字段），自动防 CRLF 日志注入。

## 导出 API

| 符号 / 方法 | 说明 |
| :--- | :--- |
| `LogfmtField` | 单字段视图：`key`、`raw_value`、`is_quoted`、`has_value`，方法 `decodeValue(out)`、`asInt()`、`asUint()`、`asFloat()`、`asBool()` |
| `LogfmtIter` | 零拷贝行迭代器：`init(line)`、`next(*_): LogfmtField?`、`hasError(): bool` |
| `LogfmtEncoder` | 原子日志构造器：`init(buf)`、`kvStr`、`kvInt`、`kvUint`、`kvFloat`、`kvBool`、`kvFlag`、`endLine`、`written()`、`reset()` |
| `getLogfmtValue(line, key, out)` | 按键查找并解转义字符串值到 `out` |
| `getLogfmtInt(line, key): i64?` | 按键查找并解析为 `i64` |
| `getLogfmtBool(line, key): bool?` | 按键查找并解析为 `bool` |
| `countLogfmtFields(line): usize` | 统计合法字段个数 |
| `appendLogfmtPair(out, pos, key, val)` | 原子追加单个 `key=value`（含自动加引号与转义） |

## 安全与边界特性

1. **防 CRLF 日志注入**：包含空白、`=`、`"`、`\` 或 `\r`/`\n`/`\t` 的值会被自动加双引号并转义为 `\"`、`\\`、`\n`、`\r`、`\t`，彻底杜绝恶意输入伪造多行日志。
2. **原子写入保护**：`appendLogfmtPair` 与 `LogfmtEncoder` 在缓冲区容量不足或键名非法时自动回滚写入游标，确保已写入的前序字段保持完整合法。
3. **畸形行告警**：遇到未闭合双引号或裸值内非法 `=`/`"` 时，`LogfmtIter.next()` 立即中止并置 `hasError() == true`。

## 使用示例

### 示例 1：使用 `LogfmtEncoder` 构建结构化访问日志

```cp
import { LogfmtEncoder } from "cap:logfmt";

fn demoEncode() {
    let buf: [256]u8 = [0; 256];
    let enc = LogfmtEncoder.init(buf[0..]);
    * = enc.kvStr("level", "info");
    * = enc.kvStr("msg", "request handled\nwith newline");
    * = enc.kvInt("status", 200);
    * = enc.kvFloat("latency_ms", 1.25);
    * = enc.kvBool("cached", true);
    * = enc.kvFlag("production");

    const line = enc.written();
    // level=info msg="request handled\nwith newline" status=200 latency_ms=1.250 cached=true production
    * = line;
}
```

### 示例 2：零拷贝迭代与类型化字段解析

```cp
import { LogfmtIter, getLogfmtInt, getLogfmtBool } from "cap:logfmt";

fn demoParse() {
    const line = "level=warn status=503 retry=true msg=\"upstream timeout\" debug";
    const status = getLogfmtInt(line, "status").?; // 503
    const retry = getLogfmtBool(line, "retry").?;  // true
    const debug = getLogfmtBool(line, "debug").?;  // true (bare flag)

    let it = LogfmtIter.init(line);
    let val_buf: [64]u8 = [0; 64];
    while (field := it.next()) {
        const decoded = field.decodeValue(val_buf[0..]);
        * = decoded;
    }
    * = status;
    * = retry;
    * = debug;
}
```

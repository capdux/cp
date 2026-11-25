# `@cap/qs` — 零分配嵌套查询字符串解析与序列化器

> 独立 Clean-Room 纯 CP 实现（严格对标 npm `qs` [周下载 120M+] 与 Rust `serde_qs` crate）| 许可协议：MIT

## 架构概述

`@cap/qs` 是面向高并发 HTTP 服务与 API 网关设计的纯 CP、零外部依赖、零堆分配查询字符串编解码库：

- **零堆分配内联池设计**：`QsParsed` 内置 4 KiB 栈上解码字符串池（`undefined` 延迟初始化，零清零开销）与最多 64 条 `QsEntry`；未包含 `%` / `+` 的键与值直接零拷贝切片自原始输入；解码值通过池偏移安全寻址，按值返回永不悬空。
- **完整嵌套与数组语法**：
  - 多级方括号嵌套：`a[b][c]=1`
  - 可选点号路径嵌套（`allow_dots: true`）：`user.name=alice`、`a.b.c=1`
  - 显式索引与空括号数组：`arr[0]=x&arr[1]=y`、`arr[]=x&arr[]=y` 及混合写法 `arr[0]=x&arr[]=y`
  - 逗号拆分数组（`comma_arrays: true`）：`tags=a,b,c`（严格按原始未编码逗号拆分，保留 `%2C`）
  - 严格 `null` 语义（`strict_null_handling: true`）：精确区分 `foo`（`null`）与 `foo=`（空字符串 `""`）
- **资源耗尽防护**：通过 `max_params`（默认 64）与 `max_depth`（默认 8）在解析期即时阻断深层嵌套与海量参数导致的资源耗尽，并严格校验不闭合括号与非法 `%XX` 序列。
- **异常安全 `QsBuilder`**：零分配序列化至调用方栈缓冲区，支持 `?` 前缀、`space_as_plus`、`addNested2`、`addNested3`、`addArrayItem`、`addBracketArrayItem`、`addNull` 与 `reset()` 复用；缓冲区不足时自动回滚写入游标。

## 版权与许可

Copyright (c) 2026 CP-Lang Contributors. Released under the **MIT License**.

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `QsError` | 错误集：`BufferTooSmall`、`TooManyParams`、`DepthExceeded`、`InvalidPercentEncoding`、`InvalidSyntax` |
| `QsParseOptions` | 解析选项：`max_params`、`max_depth`、`allow_dots`、`comma_arrays`、`ignore_query_prefix`、`plus_as_space`、`strict_null_handling` |
| `QsPathSegment` | 路径段：`name`、`is_index`、`index`、`is_empty_bracket` |
| `QsEntry` | 单条解析结果：`raw_key`、`decoded_key`、`decoded_value`、`is_null`、`segments`、`depth` |
| `QsParsed.parse(input, opts)` | 零堆分配解析查询字符串，返回 `QsParsed ! QsError` |
| `QsParsed.parseInto(*_, input, opts)` | 就地解析并复用栈上 `QsParsed` 容器 |
| `QsParsed.get(flat_key)` | 按扁平键名查询值（不存在或 strict-null 返回 `null`） |
| `QsParsed.getNested2(k1, k2)` | 查询 2 级嵌套键（如 `user[name]` 或 `user.name`） |
| `QsParsed.getNested3(k1, k2, k3)` | 查询 3 级嵌套键（如 `a[b][c]`） |
| `QsParsed.getArrayItem(key, idx)` | 按索引查询数组项（兼容 `arr[0]`、`arr[]`、逗号数组与重复键） |
| `QsParsed.countArrayItems(key)` | 统计指定数组键的元素总数 |
| `QsParsed.isNull(flat_key)` | 判断指定键是否为 strict-null 参数 |
| `QsParsed.len()` / `entryAt(idx)` / `entryKey(idx)` / `entryValue(idx)` | 条目总数与按序安全访问 |
| `percentDecode(input, plus_as_space, out)` | 严格 `%XX` 解码（支持 `+` 转空格） |
| `percentEncode(input, space_as_plus, out)` | 严格 RFC 3986 unreserved 字符集编码 |
| `QsBuilder` (`init` / `initWithOptions` / `add` / `addNested2` / `addNested3` / `addArrayItem` / `addBracketArrayItem` / `addNull` / `written` / `reset`) | 零分配、异常安全的查询字符串构造器 |

## 使用示例

### 示例 1：解析嵌套方括号、点号路径与混合数组

```cp
import { QsParsed, QsParseOptions } from "@cap/qs";

fn demoParseComplex() {
    const query = "?a[b][c]=1&arr[0]=x&arr[]=y&tags=go,zig,cp&user.name=alice";
    let parsed = QsParsed.parse(query, QsParseOptions{
        allow_dots: true,
        comma_arrays: true,
    }) catch return;

    const deep = parsed.getNested3("a", "b", "c").?;       // "1"
    const uname = parsed.getNested2("user", "name").?;     // "alice"
    const a0 = parsed.getArrayItem("arr", 0).?;            // "x"
    const a1 = parsed.getArrayItem("arr", 1).?;            // "y"
    const t2 = parsed.getArrayItem("tags", 2).?;           // "cp"
    * = deep;
    * = uname;
    * = a0;
    * = a1;
    * = t2;
}
```

### 示例 2：严格 `null` 语义与 Percent 编解码

```cp
import { QsParsed, QsParseOptions, percentEncode, percentDecode } from "@cap/qs";

fn demoStrictNullAndPercent() {
    let parsed = QsParsed.parse("flag&empty=&msg=hello+world%21", QsParseOptions{
        strict_null_handling: true,
    }) catch return;

    const is_flag_null = parsed.isNull("flag");            // true (get("flag") == null)
    const empty_val = parsed.get("empty").?;               // "" (非 null 空串)
    const msg = parsed.get("msg").?;                       // "hello world!"

    let enc_buf: [64]u8 = undefined;
    let dec_buf: [64]u8 = undefined;
    const encoded = percentEncode("CP 语言 / 100%", false, enc_buf[0..]) catch return;
    const decoded = percentDecode(encoded, false, dec_buf[0..]) catch return;
    * = is_flag_null;
    * = empty_val;
    * = msg;
    * = decoded;
}
```

### 示例 3：使用 `QsBuilder` 零分配构造并往返解析

```cp
import { QsBuilder, QsParsed, QsParseOptions } from "@cap/qs";

fn demoBuildAndRoundTrip() {
    let buf: [256]u8 = undefined;
    let b = QsBuilder.initWithOptions(buf[0..], true, true);
    b.add("q", "hello world") catch return;
    b.addNested2("filter", "status", "active") catch return;
    b.addArrayItem("ids", 0, "101") catch return;
    b.addBracketArrayItem("ids", "202") catch return;
    b.addNull("debug") catch return;

    // "?q=hello+world&filter[status]=active&ids[0]=101&ids[]=202&debug"
    const qs_str = b.written();
    let reparsed = QsParsed.parse(qs_str, QsParseOptions{
        strict_null_handling: true,
    }) catch return;
    * = reparsed.get("q");
}
```

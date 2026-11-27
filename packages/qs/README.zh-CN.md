[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# `qs`

面向 CP 语言的零分配嵌套查询字符串解析与序列化器——完整对标 **npm `qs`**，支持多级方括号嵌套（`a[b][c]`）、点号路径、索引/空括号/逗号数组、严格 `null` 语义、RFC 3986 百分号编解码以及深度与参数数量上限保护。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源包平台与对标** | `npm:qs` ([ljharb/qs](https://github.com/ljharb/qs), v6 对齐) |
| **CP 包名与版本** | `qs@1.0.0` (`MIT`) |
| **外部依赖** | 无（仅依赖内置 `cap:ascii`） |

保持原包名导入，零改名成本直接迁移：

```ts
// TypeScript (npm qs)
import qs from "qs";
const obj = qs.parse("a[b][c]=1&tags=go,zig,cp", { comma: true });
```

```cp
// CP — 保持原包名，零堆分配栈上池
import { QsParsed, QsParseOptions } from "qs";

let parsed = QsParsed.parse("a[b][c]=1&tags=go,zig,cp", QsParseOptions{
    comma_arrays: true,
}) catch return;
const v = parsed.getNested3("a", "b", "c"); // "1"
```

## ReleaseFast 实测性能亮点

在 Intel Xeon Platinum 8175M 平台下使用 `cpc 1.0.0` + `zig 0.17.0 -OReleaseFast -fllvm` 实测（详见 [`bench/ECOSYSTEM_PERF_REPORT.md`](../../bench/ECOSYSTEM_PERF_REPORT.md)）：

| 操作 | 测试载荷 | 延迟 | 吞吐量 | 带宽 | 核心优化架构 |
| :--- | :--- | --: | --: | --: | :--- |
| `percentDecode` | 118 B 百分号编码串 | **170.40 ns/op** | **5,868,609 ops/s** | **660.42 MB/s** | 16-lane SIMD 扫描 |
| `QsParsed.parseInto` | 91 B（8 组嵌套/数组参数） | **367.35 ns/op** | **2,722,234 ops/s** | 236.25 MB/s | 16-lane SIMD + 零清零栈池 |

- **零栈清零内联池**：`QsParsed` 内置 4 KiB 栈上解码池（`POOL_CAPACITY = 4096`）与 64 条目槽位（`MAX_PARAMS = 64`），采用 `undefined` 延迟初始化，彻底消除栈清零开销。
- **零拷贝快路径**：不含 `%` 或 `+` 的键值直接切片自原始输入；解码内容通过偏移量寻址，按值返回 `QsParsed` 永不悬空。

## 实战示例

### 1. 解析嵌套方括号、点号路径与混合数组

```cp
import { QsParsed, QsParseOptions } from "qs";

fn demoParse() {
    const query = "?a[b][c]=1&arr[0]=x&arr[]=y&tags=go,zig,cp&user.name=alice";
    let parsed = QsParsed.parse(query, QsParseOptions{
        allow_dots: true,
        comma_arrays: true,
    }) catch return;

    const deep = parsed.getNested3("a", "b", "c").?;   // "1"
    const uname = parsed.getNested2("user", "name").?; // "alice"
    const a0 = parsed.getArrayItem("arr", 0).?;        // "x"
    const a1 = parsed.getArrayItem("arr", 1).?;        // "y"
    const cnt = parsed.countArrayItems("tags");        // 3
    _ = deep; _ = uname; _ = a0; _ = a1; _ = cnt;
}
```

### 2. 栈容器就地复用（`parseInto`）、严格 `null` 与百分号编解码

```cp
import { QsParsed, QsParseOptions, percentEncode, percentDecode } from "qs";

fn demoReuseAndStrictNull() {
    // 通过 parseInto() 在高频请求中重复使用同一份栈容器
    let parsed = QsParsed.init();
    parsed.parseInto("flag&empty=&msg=hello+world%21", QsParseOptions{
        strict_null_handling: true,
    }) catch return;

    const is_null = parsed.isNull("flag"); // true（get("flag") == null）
    const empty = parsed.get("empty").?;   // ""（非 null 空串）
    const msg = parsed.get("msg").?;       // "hello world!"

    let enc_buf: [64]u8 = undefined;
    let dec_buf: [64]u8 = undefined;
    const enc = percentEncode("CP / 100%", false, enc_buf[0..]) catch return;
    const dec = percentDecode(enc, false, dec_buf[0..]) catch return;
    _ = is_null; _ = empty; _ = msg; _ = dec;
}
```

### 3. `QsBuilder` 零分配序列化与边界错误处理（`QsError`）

```cp
import { QsBuilder, QsParsed, QsParseOptions, QsError } from "qs";

fn demoBuilderAndErrors() {
    let buf: [256]u8 = undefined;
    let b = QsBuilder.initWithOptions(buf[0..], true, true);
    b.add("q", "hello world") catch return;
    b.addNested2("filter", "status", "active") catch return;
    b.addArrayItem("ids", 0, "101") catch return;
    b.addBracketArrayItem("ids", "202") catch return;
    b.addNull("debug") catch return;
    const qs_str = b.written(); // "?q=hello+world&filter[status]=active&ids[0]=101&ids[]=202&debug"

    // 深度与参数数量上限即时拦截超限输入并返回 QsError
    let small_opts = QsParseOptions{ max_depth: 2, max_params: 16 };
    _ = QsParsed.parse("a[b][c][d]=1", small_opts) catch |err| {
        // err == QsError.DepthExceeded
        _ = err;
        return;
    };
    _ = qs_str;
}
```

## 核心 API 速览

- **错误集**：`QsError`（`BufferTooSmall`、`TooManyParams`、`DepthExceeded`、`InvalidPercentEncoding`、`InvalidSyntax`）
- **解析器**：`QsParsed`（`init`、`parse`、`parseInto`、`get`、`getNested2`、`getNested3`、`getArrayItem`、`countArrayItems`、`isNull`、`len`、`entryAt`、`entryKey`、`entryValue`）
- **构造器**：`QsBuilder`（`init`、`initWithOptions`、`add`、`addNested2`、`addNested3`、`addArrayItem`、`addBracketArrayItem`、`addNull`、`written`、`reset`）
- **百分号编解码**：`percentDecode`、`percentEncode`、`needsDecode`、`isUnreserved`

完整类型签名请查阅 [`src/index.d.cp`](src/index.d.cp)。

# cap:structured_headers

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 8941 HTTP Structured Field Values），提供纯 CP、零外部依赖、零堆分配的 SFV 标量项、参数列表解析与序列化器。

## 导入方式

```cp
import {
    SfItemKind,
    SfItem,
    SfParam,
    SfParamIter,
    parseSfItem,
    formatSfString,
    formatSfByteSeq,
} from "cap:structured_headers";
```

## 核心类型与 API

- `SfItemKind`：`integer` / `decimal_milli` / `string` / `token` / `byte_seq` / `boolean`。
- `SfItem`：`kind`、`int_val: i64`（`decimal_milli` 为千分整数如 `1.25` -> `1250`）、`bool_val: bool`、`str_val: []const u8`。
- `parseSfItem(src: []const u8, consumed: *usize): SfItem?`：支持 `?1`/`?0`、15 位整数上限、12+3 位小数、`\"`/`\\` 字符串、Token 与 `:base64:` 字节序列。
- `SfParam` & `SfParamIter`：迭代 `;k1=v1;flag;k2="str"` 参数列表，无 `=` 时默认为 `boolean(true)`。
- `formatSfString(s, out): ([]const u8)?` 与 `formatSfByteSeq(b64, out): ([]const u8)?`。

## 使用示例

```cp
import { parseSfItem, SfParamIter } from "cap:structured_headers";

fn example() {
    let used: usize = 0;
    const hdr = "foo/bar;u=3;i;q=0.75";
    if (item := parseSfItem(hdr, &used)) {
        * = item.str_val; // "foo/bar"
        let it = SfParamIter.init(hdr[used..]);
        while (it.next() != null) {}
    }
}
```

# cap:structured_headers

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

基于 RFC 8941 HTTP Structured Field Values 规范，提供纯 CP、零外部依赖、零堆分配的 SFV 标量项、参数列表解析与序列化器。

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

## 上游行为契约与 RFC 8941 精度/安全边界

1. **无浮点误差千分位定点数 (`decimal_milli`)**：
   - RFC 8941 规定小数最多 12 位整数 + 3 位小数，`SfItem` 以 `int_val: i64` 千分整数精确表示（如 `1.25 -> 1250`），彻底避免 IEEE 754 二进制浮点舍入误差；整数严格限制在 15 位以内（`[-999_999_999_999_999, 999_999_999_999_999]`）。
2. **字符串与参数列表严格文法**：
   - 字符串仅允许可见 ASCII（`0x20..=0x7E`）与 `\"`/`\\` 两种转义，拒绝控制字符与非法转义序列；
   - `SfParamIter` 遇到省略 `=value` 的标志参数（如 `;i`）自动补全为 `boolean(true)`，符合 `Priority` / `Cache-Status` 等现代 HTTP 头部规范。
3. **零堆分配序列化防护**：
   - `formatSfString` 与 `formatSfByteSeq` 自动转义/包裹引号与冒号，输出缓冲不足或含非法控制字符时安全返回 `null`。

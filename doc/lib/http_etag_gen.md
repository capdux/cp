# cap:http_etag_gen

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（HTTP Strong & Weak ETag Generator，符合 RFC 9110 §8.8.3），提供纯 CP、零外部依赖、零堆分配的静态文件弱 ETag、内容哈希强/弱 ETag 生成与语法校验库。

## 导入方式

```cp
import {
    fnv1a64,
    generateWeakStatEtag,
    generateStrongHashEtag,
    generateWeakHashEtag,
    isValidEtagSyntax,
} from "cap:http_etag_gen";
```

## 核心 API

- `fnv1a64(data: []const u8): u64`：标准 64 位 FNV-1a 哈希（offset basis `0xcbf29ce484222325`、prime `0x100000001b3`）。
- `generateWeakStatEtag(size_bytes: u64, mtime_ms: u64, out: []u8): ([]const u8)?`：生成基于文件元数据的弱 ETag `W/"<size_hex>-<mtime_hex>"`。
- `generateStrongHashEtag(body: []const u8, out: []u8): ([]const u8)?`：生成基于 FNV-1a 64-bit 内容哈希的强 ETag `"<16_lower_hex_fnv1a64>-<len_hex>"`。
- `generateWeakHashEtag(body: []const u8, out: []u8): ([]const u8)?`：生成基于 FNV-1a 64-bit 内容哈希的弱 ETag `W/"<16_lower_hex_fnv1a64>-<len_hex>"`。
- `isValidEtagSyntax(raw: []const u8): bool`：校验 RFC 9110 §8.8.3 语法（可选区分大小写 `W/` 前缀 + 双引号包裹 + 内部仅合法 `etagc` 字节 `0x21 | 0x23..0x7E | 0x80..0xFF`）。

## 使用示例

```cp
import {
    generateWeakStatEtag,
    generateStrongHashEtag,
    isValidEtagSyntax,
} from "cap:http_etag_gen";

fn example() {
    let buf: [64]u8 = undefined;
    if (weak := generateWeakStatEtag(4096, 1700000000000, &buf)) {
        * = isValidEtagSyntax(weak); // true: W/"1000-18bcecc7b00"
    }
    if (strong := generateStrongHashEtag("hello world", &buf)) {
        * = isValidEtagSyntax(strong); // true: "<16hex>-b"
    }
}
```

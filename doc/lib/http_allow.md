# cap:http_allow

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §10.2.1 `Allow` / `Access-Control-Allow-Methods` 与 §9.2 方法语义规范），提供纯 CP、零外部依赖、零堆分配的方法集合解析、位掩码查询与规范化格式化库。

## 导入方式

```cp
import {
    HttpMethodBit,
    AllowIter,
    AllowMethodSet,
    parseAllowHeader,
    isMethodAllowed,
    isSafeMethod,
    isIdempotentMethod,
} from "cap:http_allow";
```

## 核心类型与 API

- `HttpMethodBit`：标准 9 种 HTTP 方法位掩码常量（`GET`, `HEAD`, `POST`, `PUT`, `DELETE`, `CONNECT`, `OPTIONS`, `TRACE`, `PATCH`）。
- `AllowMethodSet`（`mask: u16`, `has_custom: bool`）：
  - `containsStandard(bit: u16): bool`：查询单个或组合标准方法位。
  - `isEmpty(): bool` / `countStandard(): u32`。
  - `formatStandard(out: []u8): ([]const u8)?`：按固定规范顺序输出如 `"GET, HEAD, POST, OPTIONS"`。
- `parseAllowHeader(header: []const u8): AllowMethodSet`：严格区分大小写解析逗号分隔方法列表。
- `AllowIter`：`init(header)` / `next(*_): ([]const u8)?`，流式迭代合法 RFC 9110 token 方法名（支持 `PROPFIND`/`MKCOL` 等扩展方法）。
- `isMethodAllowed(header: []const u8, method: []const u8): bool`：区分大小写精确匹配标准与扩展方法。
- `isSafeMethod(method: []const u8): bool`（`GET`/`HEAD`/`OPTIONS`/`TRACE`）与 `isIdempotentMethod(method: []const u8): bool`（`GET`/`HEAD`/`PUT`/`DELETE`/`OPTIONS`/`TRACE`）。

## 使用示例

```cp
import { HttpMethodBit, parseAllowHeader, isMethodAllowed } from "cap:http_allow";

fn example() {
    const hdr = "POST, GET, HEAD, PROPFIND";
    const set = parseAllowHeader(hdr);
    * = set.containsStandard(HttpMethodBit.GET | HttpMethodBit.POST); // true
    * = isMethodAllowed(hdr, "PROPFIND"); // true
    let buf: [64]u8 = undefined;
    * = set.formatStandard(&buf); // "GET, HEAD, POST"
}
```

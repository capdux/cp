# cap:http_expect

> 源码吸收自 `/data/dev/bun/src/http/http.cp`（RFC 9110 §10.1.1 HTTP `Expect` 请求头规范），提供纯 CP、零外部依赖、零堆分配的 `Expect` 头流式解析与 `100-continue` / `417 Expectation Failed` 协商判定库。

## 导入方式

```cp
import {
    ExpectItem,
    ExpectIter,
    ExpectDecision,
    hasContinueExpectation,
    hasUnknownExpectation,
    evaluateExpectHeader,
} from "cap:http_expect";
```

## 核心类型与 API

- `ExpectItem`：`token: []const u8`、`value: ([]const u8)?`（兼容 RFC 7231 `token=value` / `token="quoted"` 扩展形式）：
  - `isContinue(): bool`：大小写不敏感匹配不带值的 `"100-continue"`。
- `ExpectIter`：`init(header: []const u8)` / `next(*_): ExpectItem?`（逗号分隔列表，引号感知，自动忽略空项与多余空白）。
- `ExpectDecision`：`none`、`send_100_continue`、`expectation_failed_417`。
- `evaluateExpectHeader(header: []const u8, has_body: bool, http11_or_newer: bool): ExpectDecision`：
  - 空头或仅逗号/空白 -> `none`；
  - 含任意未知或畸形期望 -> `expectation_failed_417`；
  - HTTP/1.0 或无请求体 -> `none`；
  - HTTP/1.1+ 且有请求体且含 `100-continue` -> `send_100_continue`。
- `hasContinueExpectation(header: []const u8): bool` 与 `hasUnknownExpectation(header: []const u8): bool`。

## 使用示例

```cp
import { ExpectDecision, evaluateExpectHeader } from "cap:http_expect";

fn example() {
    const d1 = evaluateExpectHeader("100-continue", true, true);
    * = d1 == ExpectDecision.send_100_continue; // true

    const d2 = evaluateExpectHeader("100-continue, custom-ext=\"a,b\"", true, true);
    * = d2 == ExpectDecision.expectation_failed_417; // true
}
```

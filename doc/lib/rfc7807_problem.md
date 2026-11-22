# `cap:rfc7807_problem` — RFC 9457 / RFC 7807 HTTP Problem Details 构造与序列化库

> 参考来源：/data/dev/bun/src/http/http.cp | 吸收日期：2026-11-22

## 概述

`cap:rfc7807_problem` 吸收自 Bun HTTP 协议基础设施（`src/http/http.cp`），实现符合 RFC 9457（替代 RFC 7807）标准的 `application/problem+json` 结构化 HTTP 错误响应构造与零堆分配 JSON 序列化，确保微服务与 REST API 能够以机器可读且 RFC 8259 合规的格式返回错误详情。

## 规范契约与安全边界

- **标准媒体类型**：统一通过 `PROBLEM_JSON_CONTENT_TYPE = "application/problem+json"` 设置 `Content-Type` 响应头。
- **RFC 8259 严格转义**：`toJson(out)` 对 `type`、`title`、`detail`、`instance` 等字符串字段执行完整 JSON 转义（双引号、反斜杠、换行、制表符与 `0x00..0x1F` 控制字符），防止错误详情中包含用户输入时发生 JSON 注入。
- **默认 `about:blank` 语义**：`ProblemDetails.fromStatus(status, detail)` 自动填充标准 `about:blank` 类型 URI 与对应 HTTP 状态码的规范英文短语（`defaultTitleForStatus(status)`），简化常见 4xx / 5xx 错误构造。
- **零堆分配输出**：写入调用方提供的栈缓冲 `out: []u8`，空间不足时原子返回 `null` 而不截断产生非法 JSON。

## 导出 API

| 符号 | 签名 / 说明 |
| :--- | :--- |
| `PROBLEM_JSON_CONTENT_TYPE` | `"application/problem+json"` 常量 |
| `defaultTitleForStatus(status: u16): []const u8` | 返回常见 HTTP 状态码的标准原因短语（如 `404` → `"Not Found"`） |
| `ProblemDetails.fromStatus(status, detail)` | 使用 `about:blank` 与默认标题构造标准错误详情 |
| `ProblemDetails.custom(type_uri, title, status, detail, instance)` | 构造带自定义问题类型 URI 与 `instance` 标识的错误详情 |
| `ProblemDetails.toJson(out: []u8): ([]const u8)?` | 零堆分配序列化为合法 RFC 8259 JSON，缓冲不足返回 `null` |

## 使用示例

```cp
import { PROBLEM_JSON_CONTENT_TYPE, ProblemDetails } from "cap:rfc7807_problem";

fn demo() {
    const p = ProblemDetails.fromStatus(429, "Rate limit exceeded; retry after 60s");
    let buf: [256]u8 = undefined;
    if (body := p.toJson(buf[0..])) {
        * = PROBLEM_JSON_CONTENT_TYPE;
        * = body;
    }
}
```

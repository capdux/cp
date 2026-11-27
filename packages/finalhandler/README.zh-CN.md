[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# finalhandler

纯 CP 实现的零堆 HTTP 终态响应与错误处理库，内置 XSS 安全 HTML 转义、默认安全响应头（`CSP`、`X-Content-Type-Options`）以及 `HEAD`/环境感知的 HTTP/1.1 响应构建。

- **源包对齐**：参考 npm [`finalhandler`](https://github.com/pillarjs/finalhandler)（`MIT`）净室重构。
- **包版本**：`1.0.0`（依赖 `escape-html`、`http`、`itoa`）。

## 安装与导入

```bash
cap add finalhandler
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "finalhandler": "^1.0.0"
  }
}
```

```typescript
import {
    FinalEnv, ResponseHeader, FinalError, FinalHandlerOptions, FinalResponse,
    normalizeErrorStatus, statusCode, getErrorStatusCode, statusReason,
    escapeHtmlText, formatErrorMessage, formatErrorHtml, createHtmlDocument,
    defaultSecurityHeaders, formatCannotMessage,
    buildFinalHttpResponse, buildFinalHttpResponseWithOptions
} from "finalhandler";
```

## 核心 API 参考

### 枚举与数据结构

| 符号 | 字段 / 枚举项 | 说明 |
| :--- | :--- | :--- |
| `FinalEnv` | `Development`, `Production` | 控制是否暴露错误详情与堆栈信息 |
| `ResponseHeader` | `name: string`、`value: string` | HTTP 响应头键值对 |
| `FinalError` | `status: u16`、`status_code: u16`、`expose: bool`、`message: string`、`stack: string` | HTTP 错误描述对象 |
| `FinalHandlerOptions` | `env: FinalEnv`、`is_head: bool`、`method: string`、`url: string`；`FinalHandlerOptions.default()` | `buildFinalHttpResponseWithOptions` 配置项 |
| `FinalResponse` | `status: u16`、`body_len: usize`、`total_len: usize` | 写入 `out` 的完整 HTTP/1.1 响应元信息 |

### 核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `normalizeErrorStatus` | `normalizeErrorStatus(code: u16): u16` | 将非 4xx/5xx 状态码规范化为 `500` |
| `statusCode` / `getErrorStatusCode` | `statusCode(err: FinalError): u16` | 从 `err.status` 或 `err.status_code` 解析规范化状态码 |
| `statusReason` | `statusReason(code: u16): string` | 返回标准 HTTP 状态原因短语（如 `404` -> `"Not Found"`） |
| `escapeHtmlText` | `escapeHtmlText(input: string, out: []u8): usize` | 将文本做 XSS 安全 HTML 实体转义并写入 `out` |
| `formatErrorMessage` | `formatErrorMessage(err: FinalError, status: u16, env: FinalEnv): string` | 开发环境优先输出详情/堆栈，生产环境隐藏内部错误 |
| `formatErrorHtml` / `createHtmlDocument` | `createHtmlDocument(message: string, out: []u8): usize` | 生成转义后的 HTML5 错误页（`<!DOCTYPE html>...<pre>...</pre>...`） |
| `defaultSecurityHeaders` | `defaultSecurityHeaders(out: []ResponseHeader): usize` | 填充默认安全头（`Content-Security-Policy`、`X-Content-Type-Options` 等） |
| `formatCannotMessage` | `formatCannotMessage(method: string, url: string, out: []u8): usize` | 格式化默认 404 文案 `"Cannot <METHOD> <URL>"` |
| `buildFinalHttpResponse` | `buildFinalHttpResponse(status_code: u16, message: string, is_head: bool, out: []u8): FinalResponse` | 构建完整 HTTP/1.1 响应（`is_head == true` 时省略正文） |
| `buildFinalHttpResponseWithOptions` | `buildFinalHttpResponseWithOptions(err: FinalError?, opts: FinalHandlerOptions, extra_headers: &[]ResponseHeader, out: []u8): FinalResponse` | 完整处理器：`err == null` 生成 404，否则按环境与附加头生成错误响应 |

## 使用示例

### 1. 构建带附加头的错误响应

```typescript
import {
    FinalEnv, FinalError, FinalHandlerOptions, ResponseHeader,
    buildFinalHttpResponseWithOptions
} from "finalhandler";

fn renderError(out: []u8): usize {
    let mut opts = FinalHandlerOptions.default();
    opts.env = FinalEnv.Production;
    opts.method = "GET";
    opts.url = "/secret";

    let err = FinalError {
        status: 403,
        status_code: 0,
        expose: true,
        message: "Access <denied>",
        stack: "internal stack trace"
    };
    let extra = [ResponseHeader { name: "X-Request-Id", value: "req-42" }];
    let res = buildFinalHttpResponseWithOptions(err, opts, &extra, out);
    return res.total_len;
}
```

### 2. 处理 `HEAD` 请求的默认 404 响应（`err == null`）

```typescript
import { FinalHandlerOptions, ResponseHeader, buildFinalHttpResponseWithOptions } from "finalhandler";

fn handleNotFoundHead(out: []u8): u16 {
    let mut opts = FinalHandlerOptions.default();
    opts.is_head = true;
    opts.method = "HEAD";
    opts.url = "/missing";

    let empty: [0]ResponseHeader = [];
    let res = buildFinalHttpResponseWithOptions(null, opts, &empty, out);
    // res.status == 404，保留 Content-Length，正文省略
    return res.status;
}
```

## 与源包差异与 CP 安全亮点

- **零堆 HTTP/1.1 拼装**：状态行、默认安全头、`Content-Length` 与转义后的 HTML 正文全部直写调用方 `out: []u8`。
- **默认安全防护**：自动注入 `Content-Security-Policy: default-src 'none'` 与 `X-Content-Type-Options: nosniff`，并经 `escape-html` 转义所有动态错误文本。
- **显式可选错误类型**：以 `err: FinalError?` 区分 404 兜底与错误响应，避免弱类型回调隐患。

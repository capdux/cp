# `cap:gemini` — Gemini Protocol v0.16.1 与 Gemtext 零分配编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

纯 CP、零堆分配（zero-alloc）的 Gemini Protocol v0.16.1 请求/响应头编解码库与 `text/gemini`（Gemtext）流式文档解析器。

## 导入

```cp
import {
    GEMINI_TCP_PORT,
    GEMINI_MAX_URL_LEN,
    GEMINI_MAX_META_LEN,
    GEMINI_DEFAULT_MIME,
    STATUS_INPUT,
    STATUS_SENSITIVE_INPUT,
    STATUS_SUCCESS,
    STATUS_REDIRECT_TEMP,
    STATUS_REDIRECT_PERM,
    STATUS_TEMP_FAILURE,
    STATUS_SERVER_UNAVAILABLE,
    STATUS_CGI_ERROR,
    STATUS_PROXY_ERROR,
    STATUS_SLOW_DOWN,
    STATUS_PERM_FAILURE,
    STATUS_NOT_FOUND,
    STATUS_GONE,
    STATUS_PROXY_REFUSED,
    STATUS_BAD_REQUEST,
    STATUS_CLIENT_CERT_REQUIRED,
    STATUS_CERT_NOT_AUTHORIZED,
    STATUS_CERT_NOT_VALID,
    GeminiStatusClass,
    GemtextLineKind,
    GeminiRequest,
    parseGeminiRequest,
    encodeGeminiRequest,
    GeminiResponseHeader,
    parseGeminiResponseHeader,
    encodeGeminiResponseHeader,
    GemtextLine,
    GemtextIter,
} from "cap:net";
```

## 核心 API

- **协议常量与状态码**：
  - `GEMINI_TCP_PORT = 1965`、`GEMINI_MAX_URL_LEN = 1024`、`GEMINI_MAX_META_LEN = 1024`、`GEMINI_DEFAULT_MIME = "text/gemini; charset=utf-8"`
  - 状态码常量：`STATUS_INPUT (10)` .. `STATUS_CERT_NOT_VALID (62)`
  - 枚举：`GeminiStatusClass: u8`（`input=1`, `success=2`, `redirect=3`, `temp_failure=4`, `perm_failure=5`, `client_cert=6`）、`GemtextLineKind: u8`（`text`, `link`, `heading1`, `heading2`, `heading3`, `list_item`, `quote`, `preformat_toggle`, `preformat_text`）
- **请求编解码**：
  - `parseGeminiRequest(line): GeminiRequest?` — 严格校验 `\r\n` 终止、`url.len <= 1024`、合法 `scheme://...`、拒绝 `#` fragment 与控制字符/空格。
  - `encodeGeminiRequest(url, out: []u8): usize?` — 零分配编码为 `<URL>\r\n`。
- **响应头编解码**：
  - `parseGeminiResponseHeader(line): GeminiResponseHeader?` — 校验 2 位状态码 `10..69`、第 3 字节为空格 `0x20`、`meta.len <= 1024`、`\r\n` 终止。
  - `GeminiResponseHeader` 方法：`statusClass()`、`isInput()`、`isSuccess()`、`isRedirect()`、`isError()`、`mimeType()`（2x 响应默认 `text/gemini; charset=utf-8`）。
  - `encodeGeminiResponseHeader(status, meta, out: []u8): usize?` — 零分配编码为 `<STATUS> <META>\r\n`。
- **Gemtext 零拷贝迭代解析**：
  - `GemtextIter`（`init`, `next`, `inPreformat`）逐行产出 `GemtextLine`（`kind`, `text`, `url`, `alt`），自动维护 `` ``` `` 预格式化块开关状态，块内所有行均作为 `preformat_text` 原样输出。

## 示例

```cp
import std from "std";
import {
    STATUS_SUCCESS,
    encodeGeminiRequest,
    parseGeminiRequest,
    encodeGeminiResponseHeader,
    parseGeminiResponseHeader,
    GemtextIter,
} from "cap:net";

fn example() {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    if (n := encodeGeminiRequest("gemini://geminiprotocol.net/", buf[0..])) {
        if (req := parseGeminiRequest(buf[0..n])) {
            * = req.url;
        }
    }

    if (hn := encodeGeminiResponseHeader(STATUS_SUCCESS, "text/gemini; charset=utf-8", buf[0..])) {
        if (hdr := parseGeminiResponseHeader(buf[0..hn])) {
            * = hdr.mimeType();
        }
    }

    let it = GemtextIter.init("# Hello Gemini\n=> gemini://example.com/ Home\n");
    while (true) {
        if (line := it.next()) {
            * = line;
        } else {
            break;
        }
    }
}
```

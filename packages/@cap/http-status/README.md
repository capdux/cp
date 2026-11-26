# @cap/http-status

Zero-allocation RFC 9110, WebDAV, and Cloudflare HTTP status code constants, bidirectional reason-phrase lookup, and semantic predicates for CP — clean-room MIT parity with npm `statuses` / `http-status-codes` and Rust `http::StatusCode`.

零堆分配的 CP HTTP 状态码双向查询与语义判断引擎，完整覆盖 69 个标准 IANA / RFC 9110 / WebDAV / Cloudflare 扩展状态码（100..526），支持大小写及分隔符无关的短语反查与 RFC 语义断言（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add @cap/http-status
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| Constants | `OK`, `CREATED`, `NO_CONTENT`, `NOT_FOUND`, `TOO_MANY_REQUESTS`, `INTERNAL_SERVER_ERROR`, ... (69 codes) | 全量 `u16` HTTP 状态码常量（覆盖 1xx~5xx + WebDAV + Cloudflare 52x） |
| `StatusClass` | `pub enum StatusClass: u8` | 状态分类枚举：`Informational` / `Success` / `Redirection` / `ClientError` / `ServerError` / `Invalid` |
| `reasonPhrase` | `(code: u16) -> string` | 根据状态码返回标准短语（如 `429` → `"Too Many Requests"`，未知码返回 `""`） |
| `codeFromPhrase` | `(phrase: string) -> u16?` | 根据短语反查状态码，忽略大小写并兼容空格/下划线/连字符（如 `"not_found"` / `"Not Found"` → `404`） |
| `statusClass` | `(code: u16) -> StatusClass` | 返回状态码所属分类 |
| `isValidStatus` | `(code: u16) -> bool` | 判断是否处于合法 HTTP 状态码区间 `100..=599` |
| `isKnownStatus` | `(code: u16) -> bool` | 判断是否为已知标准/扩展状态码 |
| `isInformational` / `isSuccess` / `isRedirect` / `isClientError` / `isServerError` | `(code: u16) -> bool` | 对应 1xx / 2xx / 3xx / 4xx / 5xx 分类断言 |
| `isEmptyBody` | `(code: u16) -> bool` | 判断响应是否不允许携带消息体（1xx、204、205、304） |
| `isRetryable` | `(code: u16) -> bool` | 判断是否属于可重试瞬态错误（408、425、429、500、502、503、504、522、524） |
| `isCacheableByDefault` | `(code: u16) -> bool` | 判断按 RFC 9110 是否默认可缓存（200、203、204、206、300、301、404、405、410、414、501） |

## Usage / 使用示例

```cp
import {
    OK,
    NOT_FOUND,
    TOO_MANY_REQUESTS,
    StatusClass,
    reasonPhrase,
    codeFromPhrase,
    statusClass,
    isRetryable,
    isEmptyBody,
    isCacheableByDefault,
} from "@cap/http-status";

const msg = reasonPhrase(TOO_MANY_REQUESTS); // => "Too Many Requests"
const code = codeFromPhrase("not_found");    // => 404 (u16?)
const cls = statusClass(OK);                 // => StatusClass.Success
const retry = isRetryable(503);              // => true
const empty = isEmptyBody(204);              // => true
const cache = isCacheableByDefault(301);     // => true
```

## License

MIT

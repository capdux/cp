# `cap:http_digest_auth` — RFC 7616 HTTP Digest Authentication 头部解析与构造库

> 来源：吸收自 `/data/dev/bun/src/http/http.cp`（RFC 7616 HTTP Digest Auth Header Parser）

`cap:http_digest_auth` 提供纯 CP、零外部依赖、零堆分配的 RFC 7616 `WWW-Authenticate: Digest ...` 服务端质询头与 `Authorization: Digest ...` 客户端凭证头解析及规范化构造能力。

## 核心类型与函数

### `DigestChallenge`

```cp
pub struct DigestChallenge {
    realm: []const u8 = "",
    nonce: []const u8 = "",
    opaque_val: ([]const u8)? = null,
    algorithm: ([]const u8)? = null,
    qop: ([]const u8)? = null,
    stale: bool = false,
    userhash: bool = false,

    static fn parse(header: []const u8): DigestChallenge?
}
```

- 校验 `Digest ` 方案前缀与必选字段 `realm`、`nonce`；支持引号内逗号与 `\"` 转义，大小写无关识别参数名及 `stale`/`userhash` 布尔值。

### `DigestCredentials` & `formatDigestHeader`

```cp
pub struct DigestCredentials {
    username: []const u8 = "",
    realm: []const u8 = "",
    nonce: []const u8 = "",
    uri: []const u8 = "",
    response: []const u8 = "",
    algorithm: ([]const u8)? = null,
    cnonce: ([]const u8)? = null,
    nc: ([]const u8)? = null,
    qop: ([]const u8)? = null,
    opaque_val: ([]const u8)? = null,

    static fn parse(header: []const u8): DigestCredentials?
}

pub fn formatDigestHeader(creds: DigestCredentials, out: []u8): ([]const u8)?
```

- `DigestCredentials.parse`：校验必选字段 `username`、`realm`、`nonce`、`uri`、`response` 及完整引号配对。
- `formatDigestHeader`：规范化输出 RFC 7616 `Digest ...` 头部，可与 `DigestCredentials.parse` 精确往返。

## 使用示例

```cp
import { DigestChallenge, DigestCredentials, formatDigestHeader } from "cap:http_digest_auth";

const ch = DigestChallenge.parse("Digest realm=\"api@example.com\", qop=\"auth, auth-int\", nonce=\"abc123\", algorithm=SHA-256").?;
```

# `cap:http_auth`

纯 CP 零堆分配 HTTP `Authorization` 认证头解析与编码库（RFC 9110 §11 / RFC 7617 Basic / RFC 6750 Bearer），吸收自 `/data/dev/bun/src/http/Headers.cp`。

## 导出符号

- `AuthScheme`：`unknown | basic | bearer | digest | aws4_hmac_sha256`
- `AuthHeader`：`parse(header)`、`bearerToken()`
- `BasicCredentials` / `decodeBasicAuth(header, buf)` / `encodeBasicAuth(user, pass, out)` / `encodeBearerAuth(token, out)`

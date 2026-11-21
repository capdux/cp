# `cap:sri` — W3C Subresource Integrity (SRI) 解析、格式化与流式校验器

源自 Bun `install/integrity.cp`，零堆分配实现 W3C SRI（`sha1-`/`sha256-`/`sha384-`/`sha512-` Base64 及 40 字符 Hex SHA-1）解析、多候选最强算法选择、格式化输出与恒定时间一次性/流式完整性校验。

## 导出符号

- `MAX_DIGEST_LEN`(64)、`SriError`、`IntegrityTag`（`sha1`/`sha256`/`sha384`/`sha512`）
- `Integrity`: `parse` / `parseSingle` / `parseSha1Hex` / `compute` / `verify` / `format` / `eql` / `digestSlice`
- `IntegrityVerifier`: `init` / `update` / `final` / `verify`（支持分块流式校验）

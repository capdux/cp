# `cap:sri` — W3C Subresource Integrity (SRI) 解析、格式化与流式校验器

> 参考来源：/data/dev/bun/src/install/integrity.cp | 吸收日期：2026-11-22

源自 Bun `install/integrity.cp`，零堆分配实现 W3C SRI（`sha1-`/`sha256-`/`sha384-`/`sha512-` Base64 及 40 字符 Hex SHA-1）解析、多候选最强算法选择、格式化输出与恒定时间一次性/流式完整性校验。

## 导出符号

- `MAX_DIGEST_LEN`(64)、`SriError`、`IntegrityTag`（`sha1`/`sha256`/`sha384`/`sha512`）
- `Integrity`: `parse` / `parseSingle` / `parseSha1Hex` / `compute` / `verify` / `format` / `eql` / `digestSlice`
- `IntegrityVerifier`: `init` / `update` / `final` / `verify`（支持分块流式校验）

## 上游行为契约与安全防御特性

1. **多算法候选最强优先（W3C SRI & npm lockfile 兼容）**：
   - `Integrity.parse` 支持空格分隔的多候选摘要（如 `"sha256-... sha512-..."`），按 `sha512 > sha384 > sha256 > sha1` 自动择优选取最强已知算法，同时兼容旧版 npm registry 40 位十六进制 `shasum`（`parseSha1Hex`）。
2. **恒定时间摘要比对防侧信道**：
   - `Integrity.verify` / `Integrity.eql` 与 `IntegrityVerifier.verify` 采用恒定时间字节比较，避免摘要前缀匹配泄露时序信息；`digestSlice()` 精确返回对应算法长度（20/32/48/64 字节）的零拷贝视图。
3. **流式大包体零拷贝校验**：
   - 校验大体积下载包（如 tarball / 模型权重）时推荐使用 `IntegrityVerifier` 分块 `update`，无需将整文件一次性加载入内存。

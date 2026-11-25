# `cap:sri` — W3C Subresource Integrity (SRI) 解析、多候选集合与流式校验器

> 参考来源：Rust [`ssri`](https://crates.io/crates/ssri) crate & `/data/dev/bun/src/install/integrity.cp` | 深度优化日期：2026-11-26

## 概述

`cap:sri` 提供零堆分配、恒定时间比对的完整 W3C Subresource Integrity（SRI）实现，支持 `sha1-` / `sha256-` / `sha384-` / `sha512-` Base64 格式、40 位 Hex SHA-1 shasum、多候选摘要集合（`IntegrityList`，严格遵循 W3C SRI §3.3.3 最强算法集合语义）以及面向大文件/下载流的分块流式校验器（`IntegrityVerifier`）。

## 导出 API

| 符号 / 方法 | 说明 |
| :--- | :--- |
| `MAX_DIGEST_LEN` (`64`), `MAX_SRI_ENTRIES` (`8`) | 最大摘要字节数（SHA-512）与单集合最大候选数 |
| `IntegrityTag` | 算法枚举：`unknown` / `sha1` / `sha256` / `sha384` / `sha512`（含 `digestLen` / `prefix` / `strength`） |
| `Integrity.parse(input)` | 解析单个或空格分隔 SRI 字符串，返回最强算法的单条 `Integrity` |
| `Integrity.parseSingle(tok)` | 解析单条 SRI token（自动剥离 W3C `?options` 后缀） |
| `Integrity.parseSha1Hex(hex)` | 解析 40 字符十六进制 SHA-1 shasum |
| `Integrity.compute(tag, data)` | 一次性计算指定算法的 SRI 摘要 |
| `Integrity.verify(data)` / `eql(other)` | 恒定时间摘要校验（防时序侧信道） |
| `Integrity.format(out)` / `toHex(out)` | 格式化为 `"<algo>-<base64>"` 或小写十六进制字符串 |
| `IntegrityList.parse(input)` | 解析完整 W3C SRI 多候选集合（最多 8 条，自动去重） |
| `IntegrityList.verify(data)` | 严格按 W3C SRI §3.3.3 仅用最强算法校验，命中任一同强度候选即通过 |
| `IntegrityList.push(item)` / `concat(other)` / `format(out)` | 动态追加、集合合并与空格拼接序列化 |
| `IntegrityVerifier` | 分块流式校验器：`init(expected)` / `update(chunk)` / `final()` / `verify()` |

## 安全与规范特性

1. **W3C SRI §3.3.3 最强算法集合校验**：当 SRI 属性包含多条摘要（如 `"sha256-... sha512-<A> sha512-<B>"`）时，`IntegrityList.verify` 只校验最强已知算法（`sha512`），且任意一条同强度候选匹配即通过，同时防止降级攻击。
2. **恒定时间比对**：`Integrity.eql` / `verify` 与 `IntegrityList.verify` 均采用按位异或累积比较，不因首字节不匹配而提前返回。
3. **零堆分配流式校验**：`IntegrityVerifier` 全部状态位于栈上，适合直接校验大体积下载包、tarball 或模型权重分块。

## 使用示例

### 示例 1：计算、格式化与一次性校验

```cp
import { Integrity, IntegrityTag } from "cap:sri";

fn demoOneShot() {
    const payload = "console.log('hello cp');";
    const digest = Integrity.compute(IntegrityTag.sha384, payload) catch return;

    let sri_buf: [128]u8 = [0; 128];
    const header = digest.format(sri_buf[0..]) catch return; // "sha384-..."

    const parsed = Integrity.parse(header) catch return;
    const ok = parsed.verify(payload); // true
    * = ok;
}
```

### 示例 2：W3C 多候选轮换集合校验（`IntegrityList`）

```cp
import { Integrity, IntegrityTag, IntegrityList } from "cap:sri";

fn demoMultiHash() {
    const d512_a = Integrity.compute(IntegrityTag.sha512, "old-build") catch return;
    const d512_b = Integrity.compute(IntegrityTag.sha512, "new-build") catch return;
    const d256 = Integrity.compute(IntegrityTag.sha256, "new-build") catch return;

    let list = IntegrityList{};
    * = list.push(d256);
    * = list.push(d512_a);
    * = list.push(d512_b);

    // W3C §3.3.3: 最强算法为 sha512，命中 d512_b 即通过；不匹配 sha512 的数据即使匹配 sha256 也会被拒绝
    const ok = list.verify("new-build"); // true
    * = ok;
}
```

### 示例 3：大文件分块流式校验（`IntegrityVerifier`）

```cp
import { Integrity, IntegrityTag, IntegrityVerifier } from "cap:sri";

fn demoStreaming() {
    const expected = Integrity.compute(IntegrityTag.sha256, "hello world") catch return;
    let verifier = IntegrityVerifier.init(expected);
    verifier.update("hello ");
    verifier.update("world");
    const ok = verifier.verify(); // true
    * = ok;
}
```

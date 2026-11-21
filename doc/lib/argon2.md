# `cap:argon2` / `cap.argon2` — RFC 9106 Argon2 内存硬化密码哈希与 PHC 字符串标准库

## 1. 概述

`cap:argon2` 提供符合 **RFC 9106** 标准的 Argon2 内存硬化密码哈希算法（支持 `Argon2d`、`Argon2i`、`Argon2id` 三种变体与版本 `v=19`），并内置完整的 **PHC (Password Hashing Competition)** 字符串格式化、解析、CSPRNG 随机盐生成、防 DoS 内存上限校验与常量时间摘要比对能力。

## 2. 核心类型

- `Argon2Variant` (`u8` 枚举):
  - `.Argon2d = 0`（数据依赖型，抗 GPU 破解最强）
  - `.Argon2i = 1`（数据独立型，抗侧信道计时攻击）
  - `.Argon2id = 2`（混合模式，密码哈希与密钥派生默认推荐）
- `Argon2Params`:
  - `variant: Argon2Variant = .Argon2id`
  - `t_cost: u32 = 2`（迭代次数，必须 `>= 1`）
  - `m_cost_kib: u32 = 19456`（内存成本 KiB，必须 `>= 8 * lanes`）
  - `lanes: u24 = 1`（并行通道数，必须 `>= 1`）
  - `out_len: usize = 32`（输出摘要字节数，必须 `>= 4`）
  - `max_m_cost_kib: u32 = 1048576`（防 DoS 最大内存上限，默认 1 GiB）
  - `secret: []const u8 = &.{}`（可选密钥）
  - `ad: []const u8 = &.{}`（可选关联数据）
- `PhcParsed`: 解析后的 PHC 结构（含 `variant`, `version`, `m_cost_kib`, `t_cost`, `lanes`, `salt`, `hash`，需调用 `deinit(alloc)` 释放）。
- `Argon2Error`: `InvalidParams`, `InvalidSalt`, `OutputTooShort`, `OutputTooLong`, `OutOfMemory`, `InvalidPhcFormat`, `UnsupportedVariant`, `UnsupportedVersion`, `MemoryLimitExceeded`, `VerificationFailed`。

## 3. API 速查

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `hashRaw` | `(alloc, password, salt, params) -> Argon2Error![]u8` | 计算原始 RFC 9106 Argon2 摘要 |
| `hashPhc` | `(alloc, password, salt, params) -> Argon2Error![]u8` | 生成 `$argon2id$v=19$m=...,t=...,p=...$salt$hash` PHC 字符串 |
| `hashPhcRandom` | `(alloc, password, params) -> Argon2Error![]u8` | 使用 CSPRNG 16B 随机盐生成 PHC 字符串 |
| `parsePhc` | `(alloc, phc_str, max_m_cost_kib) -> Argon2Error!PhcParsed` | 严格解析并校验 PHC 字符串与内存上限 |
| `verifyPhc` | `(alloc, password, phc_str) -> Argon2Error!bool` | 常量时间验证 PHC 字符串与口令是否匹配 |
| `verifyPhcWithLimit` | `(alloc, password, phc_str, max_m_cost_kib) -> Argon2Error!bool` | 指定自定义内存上限的常量时间验证 |

## 4. 示例

```cp
import { Argon2Params, hashPhcRandom, verifyPhc } from "cap:argon2";

const phc = try hashPhcRandom(alloc, "correct-horse-battery-staple", Argon2Params{
    .variant = .Argon2id,
    .t_cost = 2,
    .m_cost_kib = 19456,
    .lanes = 1,
});
defer alloc.free(phc);

const ok = try verifyPhc(alloc, "correct-horse-battery-staple", phc); // true
```

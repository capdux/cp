# `cap:siphash` — SipHash-2-4 & SipHash-1-3 抗 Hash-Flooding PRF 标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:crypto`，导入请使用 `import { ... } from "cap:crypto"`。

## 1. 概述
`cap:siphash`（动态挂载 `cap.siphash`）是纯 CP 零堆分配实现的 SipHash-2-4（64-bit / 128-bit）与 SipHash-1-3 伪随机函数（PRF）与短消息认证码标准库，专为防哈希碰撞 DoS（Hash-Flooding）与流式短消息校验设计。

## 2. 核心类型
- `SIPHASH_KEY_LEN`（`16` 字节密钥）
- `SipHashStream`：零堆流式分块增量计算器

## 3. API 速查
- `siphash24(key, data)` -> `u64`
- `siphash13(key, data)` -> `u64`
- `siphash24_128(key, data)` -> `[16]u8`
- `verifySipHash24(key, data, expected)` -> `bool`（常量时间比对）
- `SipHashStream.init24(key)` / `init13(key)` / `init24_128(key)` -> `update(chunk)` -> `final64()` / `final128()`

## 4. 示例
```cp
import { siphash24, SipHashStream } from "cap:crypto";
```

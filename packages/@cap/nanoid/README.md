# `@cap/nanoid` — 加密安全、URL 友好、零堆分配短 ID 生成器

> 独立 Clean-Room 纯 CP 实现，遵循 NanoID 规范（对标 npm `nanoid` 与 Rust `nanoid` crate）| 许可协议：MIT

## 概述

`@cap/nanoid` 提供纯 CP、零外部依赖、零堆分配的加密安全唯一 ID 生成器：
- **默认 21 字符 URL 安全字符集（`A-Za-z0-9_-`）**：碰撞概率与 UUID v4 相当（~126 bit 熵），体积更短；
- **零取模偏差（Zero Modulo Bias）**：对自定义字母表采用最小位掩码拒绝采样（`calcBitmask`），确保每个字符出现概率严格均匀；
- **OS CSPRNG 驱动**：底层调用 `getrandom` 获取密码学安全随机字节，同时提供确定性接口 `generateFromBytes` 便于单元测试与可复现构建。

## 导出 API

| 符号 / 签名 | 说明 |
| :--- | :--- |
| `DEFAULT_SIZE` (`21`), `URL_ALPHABET` | 默认长度与 64 字符 URL 安全字母表 |
| `ALPHABET_ALPHANUMERIC`, `ALPHABET_LOWER_ALPHANUMERIC`, `ALPHABET_NUMBERS`, `ALPHABET_HEX_LOWER`, `ALPHABET_NOLOOKEALIKE` | 常用预设字母表（数字、十六进制、无易混字符集等） |
| `nanoid(): [21]u8` | 栈上零分配生成标准 21 字符 URL 安全 NanoID |
| `nanoidLen(size, out): ([]const u8) ! NanoidError` | 生成指定长度的 URL 安全 NanoID（64 字符快路径，零拒绝） |
| `nanoidCustom(alphabet, size, out): ([]const u8) ! NanoidError` | 使用自定义字母表（2..=256 字节）与无偏差位掩码拒绝采样生成 ID |
| `generateFromBytes(alphabet, size, random_bytes, out)` | 基于调用方随机字节池的确定性生成（适用于测试） |
| `calcBitmask(alphabet_len): u8` | 计算最小无偏差拒绝采样位掩码 |
| `entropyBits(alphabet_len, size): f64` | 计算指定字母表与长度对应的信息熵（bit） |
| `isValidNanoid(id): bool` / `isValidCustom(id, alphabet, expected_len): bool` | 校验 ID 合法性 |

## 使用示例

### 示例 1：生成标准 21 字符安全 ID

```cp
import { nanoid, isValidNanoid } from "@cap/nanoid";

fn demoBasic() {
    const id = nanoid(); // [21]u8，例如 "V1StGXR8_Z5jdHi6B-myT"
    const ok = isValidNanoid(id[0..]); // true
    * = ok;
}
```

### 示例 2：自定义长度与无易混字符短码（邀请码 / 订单号）

```cp
import { nanoidLen, nanoidCustom, ALPHABET_NOLOOKEALIKE } from "@cap/nanoid";

fn demoCustom() {
    let buf1: [32]u8 = [0; 32];
    const token = nanoidLen(32, buf1[0..]) catch return;

    let buf2: [10]u8 = [0; 10];
    const invite_code = nanoidCustom(ALPHABET_NOLOOKEALIKE, 10, buf2[0..]) catch return;
    * = token;
    * = invite_code;
}
```

### 示例 3：确定性熵源与安全熵评估

```cp
import { generateFromBytes, entropyBits, ALPHABET_HEX_LOWER } from "@cap/nanoid";

fn demoDeterministic() {
    const seed: [8]u8 = [0x01, 0x0A, 0xFF, 0x23, 0x45, 0x67, 0x89, 0xAB];
    let out: [8]u8 = [0; 8];
    const id = generateFromBytes(ALPHABET_HEX_LOWER, 8, seed[0..], out[0..]) catch return;
    const bits = entropyBits(64, 21); // 126.0 bits
    * = id;
    * = bits;
}
```

[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# nanoid

纯 CP 实现的零堆分配、URL 安全、无模偏差（Bitmask 拒绝采样）唯一 ID 生成器，支持自定义字符表与确定性种子 PRNG。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/nanoid) |
| **原包名与版本** | `nanoid`（v5 对齐） |
| **上游仓库** | https://github.com/ai/nanoid |
| **CP 包名与版本** | `nanoid`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import { nanoid, customAlphabet } from "nanoid";

// CP (cap)
import {
    URL_ALPHABET,
    DEFAULT_SIZE,
    nanoidFromBytes,
    customAlphabetFromBytes,
    NanoIdGenerator,
    nanoidSeeded,
    isValidNanoId,
} from "nanoid";
```

## 安装

```bash
cap add nanoid
```

## 核心特性与安全亮点

- **零堆分配**：所有生成的 ID 直接写入调用方传入的 `[]u8` 缓冲区。
- **位掩码拒绝采样（零模偏差）**：自定义字符表采用 `nidBitmask` 掩码 + 拒绝采样，彻底消除 `% alphabet.len` 带来的分布偏差。
- **适配密码学随机源**：可将系统 CSPRNG 随机字节直接喂给 `nanoidFromBytes` / `customAlphabetFromBytes` 生成高强度安全 ID。
- **确定性种子生成器**：`NanoIdGenerator` 与 `nanoidSeeded` 适用于单元测试、压测夹具与可复现数据生成。

## 快速上手

### 使用随机字节生成标准 ID 与自定义字符表 ID

```typescript
import {
    DEFAULT_SIZE,
    URL_ALPHABET,
    nanoidFromBytes,
    customAlphabetFromBytes,
    isValidNanoId,
} from "nanoid";

fn main(): void {
    let entropy: [32]u8 = [
        0x1a, 0x2b, 0x3c, 0x4d, 0x5e, 0x6f, 0x70, 0x81,
        0x92, 0xa3, 0xb4, 0xc5, 0xd6, 0xe7, 0xf8, 0x09,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff, 0x01,
    ];

    let id_buf: [DEFAULT_SIZE]u8 = undefined;
    const len = nanoidFromBytes(entropy[0..], DEFAULT_SIZE, id_buf[0..]);
    _ = len;

    let hex_buf: [12]u8 = undefined;
    const hex_len = customAlphabetFromBytes("0123456789abcdef", entropy[0..], 12, hex_buf[0..]);
    _ = hex_len;

    const valid = isValidNanoId("V1StGXR8_Z5jdHi6B-myT", URL_ALPHABET);
    _ = valid;
}
```

### 确定性种子生成器

```typescript
import { NanoIdGenerator, nanoidSeeded, DEFAULT_SIZE } from "nanoid";

fn generateBatch(): void {
    let gen = NanoIdGenerator.new(42);
    let buf1: [DEFAULT_SIZE]u8 = undefined;
    let buf2: [10]u8 = undefined;

    const n1 = gen.nextInto(DEFAULT_SIZE, buf1[0..]);
    const n2 = gen.nextCustomInto("ABCDEFGHJKLMNPQRSTUVWXYZ23456789", 10, buf2[0..]);

    let buf3: [16]u8 = undefined;
    const n3 = nanoidSeeded(1337, 16, buf3[0..]);
    _ = n1;
    _ = n2;
    _ = n3;
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `URL_ALPHABET` | 64 字符 URL 安全字符表 | 默认 NanoID 字符集 |
| `DEFAULT_SIZE` | `21` | 默认长度（约 126-bit 熵） |
| `nanoidFromBytes` | `(bytes: []u8, size: usize, out: []u8) -> usize` | 从随机字节生成默认字符表 ID |
| `customAlphabetFromBytes` | `(alphabet: string, bytes: []u8, size: usize, out: []u8) -> usize` | 无偏差位掩码映射自定义字符表 |
| `NanoIdGenerator.new` | `(seed: u64) -> NanoIdGenerator` | 创建确定性 PRNG 生成器 |
| `NanoIdGenerator.nextInto` | `(size: usize, out: []u8) -> usize` | 生成下一条默认字符表 ID |
| `NanoIdGenerator.nextCustomInto` | `(alphabet: string, size: usize, out: []u8) -> usize` | 生成下一条自定义字符表 ID |
| `nanoidSeeded` | `(seed: u64, size: usize, out: []u8) -> usize` | 单次确定性生成 |
| `isValidNanoId` | `(id: string, alphabet: string) -> bool` | 校验 ID 是否仅包含指定字符集 |

## 开源协议

MIT

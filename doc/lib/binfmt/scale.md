# `cap:scale` — 纯 CP Parity SCALE 确定性二进制编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 概述

`cap:scale` 提供纯 CP、零外部依赖、零堆分配的 Substrate / Polkadot SCALE（Simple Concatenated Aggregate Little-Endian）确定性编解码器，完整支持：
- **严格规范性 `Compact<u64>`**：完整实现 4 种模式（1B / 2B / 4B / BigInt 5..9B）最小长度编码与解码，严格拒绝一切非规范编码与大整数尾零；
- **全套 SCALE 原语**：小端整数（`u8`..`i64`）、布尔、SCALE 特有单字节三态 `Option<bool>`、`Option` / `Result`（`0x00=Ok`, `0x01=Err`）/ `Enum` 标签、零拷贝 `Bytes` 与 `String`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ScaleOptionBool` | 单字节三态 `Option<bool>` 枚举：`none`（0）、`some_true`（1）、`some_false`（2） |
| `compactU64Len(v)` | 计算 `Compact<u64>` 规范最小编码字节长 |
| `ScaleWriter` | 零堆分配写入器：`init(buf)`、`written()`、`remaining()`、`writeBool`/`writeOptionBool`/`writeU8`..`writeI64`/`writeCompactU64`/`writeBytes`/`writeString`/`writeOptionTag`/`writeResultTag`/`writeEnumTag` |
| `ScaleReader` | 零拷贝游标：`init(data)`、`remaining()`、`isEof()`、`readBool`/`readOptionBool`/`readU8`..`readI64`/`readCompactU64`/`readBytes`/`readString`/`readOptionTag`/`readResultTag`/`readEnumTag` |

## 示例

```cp
import { ScaleWriter, ScaleReader } from "cap:binfmt";

fn roundTrip(buf: []u8): u64 {
    let w = ScaleWriter.init(buf);
    * = w.writeCompactU64(16384);
    * = w.writeString("polkadot");
    let r = ScaleReader.init(w.written());
    return r.readCompactU64() ?? 0;
}
```

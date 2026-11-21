# `cap:bincode` — 纯 CP Rust `bincode` v1/v2 零分配二进制编解码器

## 概述

`cap:bincode` 提供纯 CP、零外部依赖、零堆分配的 Rust `bincode` v1（Legacy FixedInt）与 v2（Standard Varint）二进制编解码器，支持 Little-Endian / Big-Endian 双端序、bincode 2.0 规范 Varint（`0..250` 单字节、`0xFB`+u16、`0xFC`+u32、`0xFD`+u64）与 ZigZag 有符号编码，以及严格的目标位宽与非法标签校验。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `BincodeEndian` / `BincodeIntEncoding` | 端序（`little`/`big`）与整数编码模式（`fixed`/`varint`） |
| `BincodeConfig` | 预设配置：`legacy()`（LE+Fixed）、`standard()`（LE+Varint）、`bigEndianFixed()`（BE+Fixed） |
| `BincodeWriter` | 零分配写入器：`init(buf, cfg)`、`initStandard(buf)`、`initLegacy(buf)`、`written()`、`remaining()`、`writeBool`/`U8`..`I64`/`F32`/`F64`/`Bytes`/`String`/`OptionTag`/`VariantIdx`/`SeqLen` |
| `BincodeReader` | 零拷贝游标：`init(data, cfg)`、`initStandard(data)`、`initLegacy(data)`、`remaining()`、`isEof()`、`readBool`/`U8`..`I64`/`F32`/`F64`/`Bytes`/`String`/`OptionTag`/`VariantIdx`/`SeqLen` |

## 示例

```cp
import { BincodeWriter, BincodeReader } from "cap:bincode";

fn roundtrip(buf: []u8): i64 {
    let w = BincodeWriter.initStandard(buf);
    if (!w.writeI64(-42) || !w.writeString("bincode")) return 0;
    let r = BincodeReader.initStandard(w.written());
    return r.readI64() ?? 0;
}
```

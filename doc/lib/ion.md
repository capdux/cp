# `cap:ion` — 纯 CP Amazon Ion 1.0 二进制格式零分配解析库

## 概述

`cap:ion` 提供纯 CP、零外部依赖、零堆分配的 Amazon Ion 1.0 二进制流解析器，完整支持：
- **IVM 1.0 探测与跳过**：识别 `0xE0 0x01 0x00 0xEA` 版本魔数；
- **大端 7-bit VarUInt / VarInt**：高位置 1 停止位解码，严格防 64-bit 溢出与截断；
- **全类型 Type Descriptor 解码**：自动跳过 NOP Pad，支持 typed null、内联 bool、`L == 14` 扩展长度、大端 `pos_int`/`neg_int`/`f32`/`f64`/`symbol`/`string`/`blob`，以及零拷贝 `IonListIter` 与 `IonStructIter`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ION_IVM_1_0`, `isIonBinary(data)` | Ion 1.0 4 字节魔数常量与探测函数 |
| `IonType` | 16 种 Ion Type Descriptor 枚举（`null_nop`..`reserved`） |
| `IonVarUInt` / `readIonVarUInt`, `IonVarInt` / `readIonVarInt` | 大端 7-bit VarUInt / 带符号 VarInt 解码 |
| `IonValue`, `IonReader` | 顶层流式值读取器（自动跳过 IVM 1.0 与 NOP Pad） |
| `ionAsBool`, `ionAsUInt`, `ionAsInt`, `ionAsFloat`, `ionAsSymbolId`, `ionAsString`, `ionAsBlob` | 强类型标量与零拷贝切片提取辅助 |
| `IonListIter`, `IonStructField`, `IonStructIter` | List/SExp 与 Struct（`field_symbol_id` + `IonValue`）零拷贝迭代器 |

## 示例

```cp
import { isIonBinary, IonReader, ionAsString } from "cap:ion";

fn readFirstString(buf: []const u8): ([]const u8)? {
    if (!isIonBinary(buf)) return null;
    let r = IonReader.init(buf);
    if (v := r.next()) {
        return ionAsString(v);
    }
    return null;
}
```

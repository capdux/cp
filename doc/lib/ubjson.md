# `cap:ubjson` — 纯 CP UBJSON Draft 12 零分配二进制编解码器

## 概述

`cap:ubjson` 提供纯 CP、零外部依赖、零堆分配的 Universal Binary JSON（UBJSON Draft 12，大端序）编解码器，支持全套类型标记、自动最小整数字宽选择（`writeCompactInt`）、`$`/`#` 优化强类型容器、`No-Op`（`'N'`）自动跳过，以及带 `MAX_UBJ_DEPTH = 64` 深度上限的递归值跳过。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `UBJ_NULL` .. `UBJ_OPT_COUNT` | 全部 UBJSON Draft 12 标记字节常量（`Z/N/T/F/i/U/I/l/L/d/D/C/S/H/[/]/{/}/$/#`） |
| `MAX_UBJ_DEPTH` | 最大嵌套深度限制（`64`） |
| `UbjsonContainerHeader` | 优化容器头：`is_object`, `elem_type: u8?`, `count: u64?` |
| `UbjsonWriter` | 零分配写入器：`init(buf)`、`written()`、`remaining()`、`writeNull`/`writeNoop`/`writeBool`/`writeInt8`/`writeUint8`/`writeInt16`/`writeInt32`/`writeInt64`/`writeCompactInt`/`writeFloat32`/`writeFloat64`/`writeChar`/`writeString`/`writeKey`/`writeArrayStart`/`writeArrayEnd`/`writeTypedArrayHeader`/`writeObjectStart`/`writeObjectEnd` |
| `UbjsonReader` | 零拷贝游标（自动跳过 `'N'`）：`init(data)`、`peekMarker`、`readNull`/`readBool`/`readInt`/`readFloat32`/`readFloat64`/`readChar`/`readString`/`readKey`/`readContainerHeader`/`skipValue` |

## 示例

```cp
import { UbjsonWriter, UbjsonReader, UBJ_UINT8 } from "cap:ubjson";

fn roundtrip(buf: []u8): i64 {
    let w = UbjsonWriter.init(buf);
    if (!w.writeObjectStart()) return 0;
    if (!w.writeKey("age") || !w.writeCompactInt(42)) return 0;
    if (!w.writeObjectEnd()) return 0;

    let r = UbjsonReader.init(w.written());
    * = r.readContainerHeader(true) ?? return 0;
    * = r.readKey() ?? return 0;
    return r.readInt() ?? 0;
}
```

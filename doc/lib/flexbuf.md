# `cap:flexbuf` — 纯 CP Google FlexBuffers 无 Schema 零拷贝解析库

## 概述

`cap:flexbuf` 提供纯 CP、零外部依赖、零堆分配的 Google FlatBuffers FlexBuffers 二进制格式解析器，完整支持：
- **根引用与类型解包**：`parseFlexRoot` 校验尾部 `[root_packed_type, root_byte_width]` 并构造零拷贝 `FlexRef`；
- **全标量与间接引用**：`asNull`、`asBool`、`asInt`（含 `indirect_int`）、`asUint`（含 `indirect_uint`）、`asFloat`（32/64 位与 `indirect_float`）、`asString`、`asKey`、`asBlob`；
- **多态/定型向量与有序 Map 二分查找**：`vecLen`/`vecGet`、`mapLen`/`mapKeyAt`/`mapValueAt`/`mapGet`（严格防下溢与越界）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FlexType`, `FlexBitWidth`, `UnpackedFlexType` | FlexBuffers 值类型与 1/2/4/8 字节位宽枚举 |
| `unpackFlexType(packed)`, `packFlexType(ft, bw)`, `flexBitWidthBytes(w)` | 打包类型字节编解码与位宽换算 |
| `parseFlexRoot(data)` | 校验并解析 FlexBuffers 根节点 `FlexRef` |
| `FlexRef` | 零拷贝值引用：标量提取（`asNull`/`asBool`/`asInt`/`asUint`/`asFloat`/`asString`/`asKey`/`asBlob`）与容器访问（`vecLen`/`vecGet`/`mapLen`/`mapKeyAt`/`mapValueAt`/`mapGet`） |

## 示例

```cp
import { parseFlexRoot } from "cap:flexbuf";

fn readUserAge(buf: []const u8): i64 {
    if (root := parseFlexRoot(buf)) {
        if (age_ref := root.mapGet("age")) {
            return age_ref.asInt() ?? 0;
        }
    }
    return 0;
}
```

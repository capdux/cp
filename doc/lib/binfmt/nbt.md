# `cap:nbt` — 纯 CP Named Binary Tag (NBT) 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 概述

`cap:nbt` 提供纯 CP、零外部依赖、零堆分配的 Named Binary Tag（NBT）结构化二进制解析能力，完整支持 Java Edition（Big-Endian）与 Bedrock Edition（Little-Endian），内置 `MAX_NBT_DEPTH = 64` 嵌套深度限制以防止异常嵌套导致的栈耗尽，并提供递归跳过（`skipPayload`）与零拷贝字段定位（`findNbtField`）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `MAX_NBT_DEPTH` | 最大嵌套深度限制常量（`64`） |
| `NbtTagType: u8` | 13 种标准 NBT 标签：`end=0`, `byte=1`, `short=2`, `int=3`, `long=4`, `float=5`, `double=6`, `byte_array=7`, `string=8`, `list=9`, `compound=10`, `int_array=11`, `long_array=12` |
| `NbtEndian` | 字节序枚举：`big`（Java）、`little`（Bedrock） |
| `NbtTagHeader` / `NbtListHeader` / `NbtFieldRef` | 标签头、列表头及零拷贝字段引用结构 |
| `NbtReader` | 零分配流式读取器：`init(data)` / `initLittle(data)`、`readRootHeader(*_)`、`nextCompoundEntry(*_)`、`readByte`/`readShort`/`readInt`/`readLong`/`readFloat`/`readDouble`、`readString`/`readByteArray`/`readIntArrayRaw`/`readLongArrayRaw`/`readListHeader`、`skipPayload(*_, tag_type)` |
| `isNbtCompound(data)` | 快速校验是否为合法 Big-Endian 根 Compound |
| `findNbtField(data, target_name)` / `findNbtFieldEndian(...)` | 在根 Compound 中安全跳过无关字段，零拷贝定位目标字段的 `tag_type`、`payload_offset`、`payload_end` 与 `payload` |

## 示例

```cp
import { NbtReader, isNbtCompound, findNbtField } from "cap:binfmt";

fn readLevelSeed(bytes: []const u8): i64 {
    if (!isNbtCompound(bytes)) return 0;
    const f = findNbtField(bytes, "seed") ?? return 0;
    let r = NbtReader.init(f.payload);
    return r.readLong() ?? 0;
}
```

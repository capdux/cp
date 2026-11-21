# `cap:marshal` — 纯 CP Ruby Marshal 4.8 二进制对象零分配解析库

## 概述

`cap:marshal` 提供纯 CP、零外部依赖、零堆分配的 Ruby Marshal 4.8 二进制对象序列化格式解析器，完整支持 Ruby 压缩有符号整数（Fixnum）解码、Ruby 1.9+ `IVAR` UTF-8 包装字符串自动解包、Symbol/Symlink/Array/Hash/Bignum/Regexp/Object/Struct 零拷贝流式读取，以及带 `MAX_MARSHAL_DEPTH = 64` 限制的安全递归跳过。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `MARSHAL_MAJOR` / `MARSHAL_MINOR` / `MAX_MARSHAL_DEPTH` | 常量：`4`、`8`、`64` |
| `TYPE_NIL` .. `TYPE_MODULE` | 全部 Marshal 4.8 类型标签常量 |
| `isMarshal(data)` / `parseMarshalHeader(data)` | 校验 2 字节版本头 `[4, 8]` |
| `readMarshalInt(data, pos): MarshalIntResult?` | 解码 Ruby Marshal 压缩有符号整数（1B 紧凑区间 `-123..122` 及 1..4B 小端二补码整数） |
| `MarshalReader` | 零拷贝游标：`init(data)`、`peekTag()`、`remaining()`、`isEof()`、`readFixnum`、`readBool`、`readNil`、`readRawBytes`、`readString`（自动解包 `'I'` IVAR 字符串）、`readSymbol`、`readSymlink`、`readArrayLen`、`readHashLen`、`skipValue` |

## 示例

```cp
import { isMarshal, MarshalReader } from "cap:marshal";

fn readFirstInt(bytes: []const u8): i32 {
    if (!isMarshal(bytes)) return 0;
    let r = MarshalReader.init(bytes);
    return r.readFixnum() ?? 0;
}
```

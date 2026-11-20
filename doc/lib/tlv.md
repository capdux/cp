// Source: cp/doc/lib/tlv.md
# `cap:tlv` — ASN.1 BER/DER Tag-Length-Value (TLV) 二进制编解码标准库

纯 CP、零 C 依赖的工业级 ASN.1 BER/DER Tag-Length-Value（TLV）二进制编解码标准库，提供严格 DER 解码（零拷贝切片视图、防深度耗尽、防非最短长度与不定长）、高编号 Base-128 Tag 编解码、最小补码整数字节规范化、OID 双向无损互转以及嵌套容器流式构建器 `TlvWriter`。

## 导入方式

```cp
import {
    TLV_MAX_DEPTH,
    TlvError,
    TagClass,
    TlvTag,
    TlvNode,
    TlvWriter,
    decodeDer,
    decodeDerWithMaxDepth,
    encodeDerNode,
    encodeOidBytes,
    decodeOid,
} from "cap:tlv";

// 或动态命名空间挂载：
import cap from "cap";
// cap.tlv.decodeDer / cap.tlv.TlvWriter / cap.tlv.encodeDerNode / cap.tlv.decodeOid
```

## 核心类型与错误

### `TlvError`
- `UnexpectedEof`：输入缓冲区截断或长度超出剩余字节
- `InvalidTag`：高编号 Base-128 Tag 存在冗余前导 `0x80`、非最小编码（`< 31` 使用多字节）或 `u32` 溢出
- `InvalidLength`：长度字段保留字（`0xFF`）、超出 `usize` 范围或 `TlvWriter` 嵌套未闭合
- `NonCanonicalLength`：DER 长形式存在前导 `0x00` 或长度 `< 128` 却使用了长形式编码
- `IndefiniteLengthNotAllowed`：DER 严禁不定长编码 `0x80`
- `TrailingBytes`：根节点解码完成后存在多余尾部字节
- `MaxDepthExceeded`：嵌套深度超过 `max_depth`（默认 `TLV_MAX_DEPTH = 32`）
- `InvalidBoolean`：DER BOOLEAN 长度非 1 或值非 `0x00`/`0xFF`
- `InvalidInteger`：DER INTEGER 为空或存在非最小补码前导 `0x00`/`0xFF`
- `TagMismatch`：`TlvNode` 类型辅助方法与节点 Tag 不匹配
- `InvalidOid`：点分 OID 字符串或 DER OID 编码非法
- `OutOfMemory`：内存分配失败

### `TagClass` & `TlvTag`
- `TagClass`：`Universal = 0`、`Application = 1`、`ContextSpecific = 2`、`Private = 3`
- `TlvTag`：`tag_class: TagClass`、`constructed: bool`、`number: u32`
  - 构造：`TlvTag.init(tag_class, constructed, number)`、`TlvTag.universal(number, constructed)`、`TlvTag.application(number, constructed)`、`TlvTag.contextSpecific(number, constructed)`、`TlvTag.privateTag(number, constructed)`
  - 编解码：`tag.encodedLen()`、`tag.encodeTo(buf)`、`TlvTag.decode(bytes)`（支持 `0..30` 单字节及 Base-128 高编号多字节 Tag）

### `TlvNode`
- 字段：`tag: TlvTag`、`raw_value: []const u8`（零拷贝切片视图）、`children: []TlvNode`
- 生命周期：`node.deinit(alloc)` 递归释放所有子节点切片
- 查询与类型视图：
  - `findChild(tag_number: u32): TlvNode?`
  - `asBoolean(): bool ! TlvError`
  - `asIntegerBytes(): ([]const u8) ! TlvError`
  - `asU64(): u64 ! TlvError`
  - `asOctetString(): ([]const u8) ! TlvError`
  - `asUtf8String(): ([]const u8) ! TlvError`

## 严格 DER 解码与节点编码

- `decodeDer(alloc, bytes): TlvNode ! TlvError`（默认 `TLV_MAX_DEPTH = 32`）
- `decodeDerWithMaxDepth(alloc, bytes, max_depth): TlvNode ! TlvError`
- `encodeDerNode(alloc, node): ([]u8) ! TlvError`

## 流式 DER 构建器 `TlvWriter` & OID 编解码

- `TlvWriter.init(alloc)` / `deinit(*_)` / `reset(*_)` / `bytes()` / `toOwnedSlice(*_)`
- `writeRaw(*_, tag, value)`
- `writeBoolean(*_, val)` / `writeNull(*_)`
- `writeIntegerBytes(*_, bytes)`：剔除冗余前导 `0x00`/`0xFF`，保持最小 DER 补码表示
- `writeUnsignedIntegerBytes(*_, bytes)`：剔除多余前导 `0x00`，并在最高位为 `1` 时自动补齐前导 `0x00`
- `writeOctetString(*_, data)` / `writeUtf8String(*_, s)`
- `writeOid(*_, oid_str)`：将点分 OID（如 `"1.2.840.113549.1.1.11"`）编码为 DER OID
- `beginSequence(*_)` / `beginSet(*_)` / `beginConstructed(*_, tag)` / `endConstructed(*_)`：自动回填规范 DER 长度
- `encodeOidBytes(alloc, oid_str)` & `decodeOid(alloc, bytes)`：点分 OID 字符串与 DER OID 字节双向无损转换

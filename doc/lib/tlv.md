// Source: cp/doc/lib/tlv.md
# `cap:tlv` — 二进制 TLV 编解码、流式分帧与 Castagnoli CRC32C 校验引擎

纯 CP、零 C 依赖的工业级二进制 TLV（Tag-Length-Value）编解码器、黏包/半包流式分帧器（`LengthPrefixedFramer`）与 Castagnoli CRC32C 完整性校验库，适用于网络传输协议、IoT 嵌入式报文与 RPC 底层帧。

## 导入方式

```cp
import {
    TLV_MAX_DEPTH,
    TlvError,
    FrameHeaderKind,
    TlvNestToken,
    TlvField,
    TlvWriter,
    TlvReader,
    LengthPrefixedFramer,
    crc32c,
} from "cap:tlv";

// 或动态命名空间挂载：
import cap from "cap";
// cap.tlv.TlvWriter / cap.tlv.TlvReader / cap.tlv.LengthPrefixedFramer / cap.tlv.crc32c
```

## 线格式契约

- **TLV 记录格式**：`Tag (u16 BE, 2B)` + `Length (u32 BE, 4B)` + `Value (Length B)`
- **最大嵌套深度**：`TLV_MAX_DEPTH = 32`
- **分帧器格式**：`Header (2B U16Be / 4B U32Be / 4B U32Le)` + `Payload (Length B)` + `[可选 CRC32C (4B u32 BE)]`

## 错误与枚举

### `TlvError`
- `Truncated`：缓冲区剩余字节不足以解析 TLV 头部或完整载荷
- `TagNotFound`：`findTag` 未找到指定 Tag
- `InvalidLength`：长度超出 `u32` 范围或嵌套闭合令牌不匹配
- `FrameTooLarge`：帧载荷超过 `max_frame_bytes` 或 `FrameHeaderKind` 上限
- `InvalidChecksum`：CRC32C 校验和不匹配（载荷被篡改）
- `MaxDepthExceeded`：嵌套容器深度超过 `TLV_MAX_DEPTH`（32）
- `TypeMismatch`：字段长度或编码格式与目标类型不符（如 `asU64` 非 8 字节、`asBool` 非 `0/1`、`asStr` 非合法 UTF-8）
- `OutOfMemory`：内存分配失败

### `FrameHeaderKind`
- `U16Be`：2 字节大端无符号长度头
- `U32Be`：4 字节大端无符号长度头
- `U32Le`：4 字节小端无符号长度头

## Castagnoli CRC32C

- `crc32c(data: []const u8): u32`
  - 使用 Castagnoli `0x82F63B78` 多项式编译期查找表，与 RocksDB / gRPC / SCTP 标准一致（`crc32c("123456789") == 0xE3069283`）。

## TLV 编解码 (`TlvWriter` & `TlvReader` & `TlvField`)

### `TlvWriter`
- `TlvWriter.init(alloc: std.mem.Allocator): TlvWriter`
- `deinit(*_)` / `reset(*_)`
- `bytes(): []const u8` / `toOwnedSlice(*_): ([]u8) ! TlvError`
- `writeBytes(*_, tag: u16, data: []const u8): ! TlvError`
- `writeU64(*_, tag: u16, v: u64): ! TlvError`
- `writeI64(*_, tag: u16, v: i64): ! TlvError`
- `writeStr(*_, tag: u16, s: []const u8): ! TlvError`
- `writeBool(*_, tag: u16, b: bool): ! TlvError`
- `beginNested(*_, tag: u16): TlvNestToken ! TlvError`
- `endNested(*_, token: TlvNestToken): ! TlvError`

### `TlvReader` & `TlvField`
- `TlvReader.init(data: []const u8): TlvReader`
- `TlvReader.initWithDepth(data: []const u8, depth: usize): TlvReader ! TlvError`
- `reset(*_)` / `remaining(): usize`
- `next(*_): TlvField? ! TlvError`：顺序迭代读取下一个 TLV 字段，到达末尾返回 `null`
- `findTag(tag: u16): TlvField ! TlvError`：扫描查找指定 Tag，未找到返回 `TlvError.TagNotFound`
- `TlvField` 视图方法：
  - `asBytes(): []const u8`
  - `asU64(): u64 ! TlvError`
  - `asI64(): i64 ! TlvError`
  - `asBool(): bool ! TlvError`
  - `asStr(): ([]const u8) ! TlvError`
  - `reader(): TlvReader ! TlvError`：进入嵌套容器（自动递增深度并校验 `TLV_MAX_DEPTH`）

## 流式分帧器 (`LengthPrefixedFramer`)

- `LengthPrefixedFramer.init(alloc, kind: FrameHeaderKind, max_frame_bytes: usize, with_crc32c: bool): LengthPrefixedFramer`
- `deinit(*_)` / `bufferedBytes(): usize`
- `encodeFrame(alloc, payload: []const u8): ([]u8) ! TlvError`：打包带长度头与可选尾随 CRC32C 的完整帧
- `feed(*_, chunk: []const u8): ! TlvError`：喂入任意分片/黏包数据
- `nextFrame(*_, alloc): ([]u8)? ! TlvError`：尝试提取下一完整帧；半包返回 `null`，超限返回 `FrameTooLarge`，篡改返回 `InvalidChecksum`

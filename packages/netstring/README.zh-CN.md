[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# netstring

纯 CP 实现的零堆分配 D. J. Bernstein Netstring（`<len>:<payload>,`）帧编码器、校验器与流式解码器。

- **包名**：`netstring`（`v1.0.0`）
- **上游对齐**：洁净室移植自 `crates.io/netstring`（`MIT`），适用于 SCGI、二进制 RPC 及字节缓冲区上的零拷贝分帧。

## 安装与导入

```bash
cap add netstring
```

```typescript
import {
    NetstringFrame, NetstringDecoder,
    encodedLen, encode, encodeString,
    decode, decodeNext, countFrames, isValidNetstring,
} from "netstring";
```

## 核心 API

### 帧视图与流式解码器

| 类型 | 字段与方法 | 说明 |
| --- | --- | --- |
| `NetstringFrame` | `status: i32`, `payload_start: usize`, `payload_len: usize`, `consumed: usize`；`isOk(): bool`, `isIncomplete(): bool`, `isInvalid(): bool`, `payload(buf: &[]u8): &[]u8` | 零拷贝解码帧元数据；`payload(buf)` 直接返回原缓冲区切片视图。 |
| `NetstringDecoder` | `offset: usize`；`new(): NetstringDecoder`, `next(buf: &[]u8): NetstringFrame`, `position(): usize`, `reset()` | 带游标的流式解码器，用于连续遍历缓冲区中的多个帧。 |

### 编解码与校验函数

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `encodedLen` | `(payload_len: usize): usize` | 计算 `<len>:<payload>,` 编码后的精确字节数。 |
| `encode` | `(payload: &[]u8, dst: []u8): i64` | 将字节载荷编码写入 `dst`，返回写入字节数；缓冲区不足返回 `-1`。 |
| `encodeString` | `(payload: string, dst: []u8): i64` | 将字符串编码写入 `dst`，返回写入字节数；缓冲区不足返回 `-1`。 |
| `decode` | `(buf: &[]u8): NetstringFrame` | 从偏移 `0` 解码首个帧。 |
| `decodeNext` | `(buf: &[]u8, offset: usize): NetstringFrame` | 从指定 `offset` 解码下一帧。 |
| `countFrames` | `(buf: &[]u8): i64` | 统计 `buf` 中完整合法帧数量；若格式错误返回 `-1`。 |
| `isValidNetstring` | `(buf: &[]u8): bool` | 判断 `buf` 是否恰好为单个合法 Netstring 且无多余尾字节。 |

## 使用示例

### 1. 编码与零拷贝解码

```typescript
import { encodeString, decode } from "netstring";

fn main() {
    let wire: [64]u8 = undefined;
    let n = encodeString("hello world!", wire[0..]); // 写入 "12:hello world!,"
    if (n > 0) {
        let frame = decode(wire[0..n]);
        if (frame.isOk()) {
            let body = frame.payload(wire[0..n]); // 零拷贝切片 "hello world!"
        }
    }
}
```

### 2. 使用 `NetstringDecoder` 流式解包多帧

```typescript
import { NetstringDecoder } from "netstring";

fn parseStream(buf: &[]u8): usize {
    let dec = NetstringDecoder.new();
    let seen: usize = 0;
    while (dec.position() < buf.len) {
        let f = dec.next(buf);
        if (!f.isOk()) { break; }
        let payload = f.payload(buf);
        seen += 1;
    }
    return seen;
}
```

### 3. 长度预估与合法性校验

```typescript
import { encodedLen, isValidNetstring } from "netstring";

fn main() {
    let needed = encodedLen(12); // 16 字节（"12:<12 bytes>,"）
    let valid = isValidNetstring("5:hello,"); // true
}
```

## 与源包差异及零拷贝安全亮点

- **零拷贝载荷视图**：`NetstringFrame` 仅保存偏移与长度（`payload_start`、`payload_len`），不分配堆内存；`frame.payload(buf)` 直接借用输入缓冲区切片。
- **严格协议校验**：拒绝带前导零的长度字段（如 `05:hello,`）、缺失 `:` 或结尾 `,`，并明确区分 `isIncomplete()`（半包需继续读取）与 `isInvalid()`（畸形帧）。
- **无异常错误处理**：编码缓冲区不足返回 `-1`，解码通过 `NetstringFrame` 状态方法安全判定。

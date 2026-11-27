[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# netstring

零堆分配的纯 CP D. J. Bernstein Netstring（`<len>:<payload>,`）帧编码器、校验器与流式解码器，适用于 SCGI、进程间通信与二进制长度前缀协议。

## 源包溯源与零成本迁移

| 项目 | 说明 |
| :--- | :--- |
| **源平台** | crates.io |
| **上游原包** | `netstring` |
| **CP 包名** | `netstring` (`v1.0.0`) |
| **许可证** | MIT |

CP 生态保持原包名 `netstring` 不变，无需 `@cap/*` 前缀或改名：

```rust
// Rust (crates.io: netstring)
let frame = format!("{}:{},", payload.len(), payload);
```

```typescript
// CP（保持原包名 "netstring"）
import { encodeString, decode } from "netstring";
let mut buf: [64]u8 = undefined;
let n = encodeString("hello world!", buf[0..64]); // "12:hello world!,"
```

## 安装

```bash
cap add netstring
```

## 快速上手与实战示例

### 1. 编码字节切片与字符串到栈缓冲区

```typescript
import { encodedLen, encode, encodeString } from "netstring";

fn encodeExamples(): void {
    // 预先计算精确编码长度
    let need = encodedLen(12); // "12:" (3) + 12 + "," (1) = 16

    let mut buf: [64]u8 = undefined;
    let n1 = encodeString("hello world!", buf[0..64]); // 16 ("12:hello world!,")

    let raw: [4]u8 = [0x01, 0x02, 0x00, 0xFF];
    let n2 = encode(raw[0..4], buf[0..64]); // 7 ("4:\x01\x02\x00\xFF,")
}
```

### 2. 零拷贝解码、合法性校验与帧计数

```typescript
import { decode, decodeNext, countFrames, isValidNetstring } from "netstring";

fn decodeExamples(wire: &[]u8): void {
    if (!isValidNetstring(wire)) {
        return;
    }

    let frame = decode(wire);
    if (frame.isOk()) {
        // 返回指向 wire 内部的零拷贝视图
        let body = frame.payload(wire);
        // 从 frame.consumed 处继续解码下一帧
        let next_frame = decodeNext(wire, frame.consumed);
    }

    // 统计缓冲区中完整帧数量（非法帧返回 -1）
    let total = countFrames(wire);
}
```

### 3. 有状态流式解码器（`NetstringDecoder`）

```typescript
import { NetstringDecoder } from "netstring";

fn parseStream(wire: &[]u8): void {
    let mut dec = NetstringDecoder.new();
    while (dec.position() < wire.len) {
        let frame = dec.next(wire);
        if (frame.isOk()) {
            let body = frame.payload(wire);
            // 处理零拷贝负载
        } else if (frame.isIncomplete()) {
            // 等待更多网络字节到达
            break;
        } else {
            // frame.isInvalid() — 长度前缀非法或缺少尾部逗号 ','
            break;
        }
    }
    dec.reset();
}
```

## API 速览

### 常量
- `NS_OK: i32`（`0`）、`NS_INCOMPLETE: i32`（`1`）、`NS_INVALID: i32`（`-1`）

### 核心类型
- `NetstringFrame { status, payload_start, payload_len, consumed }`：`isOk()`、`isIncomplete()`、`isInvalid()`、`payload(buf: &[]u8): &[]u8`
- `NetstringDecoder { offset }`：`new()`、`next(buf: &[]u8): NetstringFrame`、`position(): usize`、`reset()`

### 独立函数
- `encodedLen(payload_len: usize): usize`
- `encode(payload: &[]u8, dst: []u8): i64`、`encodeString(payload: string, dst: []u8): i64`（缓冲区不足返回 `-1`）
- `decode(buf: &[]u8): NetstringFrame`、`decodeNext(buf: &[]u8, offset: usize): NetstringFrame`
- `countFrames(buf: &[]u8): i64`、`isValidNetstring(buf: &[]u8): bool`

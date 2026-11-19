# CP 标准库：RFC 6455 WebSocket 帧协议与掩码编解码手册 (`cap:ws`)

## 一、 架构总览与设计准则

`cap:ws` 是 CP 语言官方内置的零分配 RFC 6455 WebSocket 协议帧编解码标准库，提供完整的操作码与关闭状态码枚举、32 位循环异或掩码引擎、自适应长度（7-bit / 16-bit / 64-bit）帧头编解码器以及全套数据帧与控制帧构造器。核心实现吸收自 Bun 原生 WebSocket 客户端子系统（`/data/dev/bun/src/http_jsc/websocket_client/`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP WebSocket 帧处理管线                            │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 协议语义层 (opcode)   │  帧头编解码 (FrameHeader)│  帧构造与掩码层         │
│ Opcode (u4)          │  encode(out) -> !usize  │  FrameBuilder          │
│   isControl/isData   │  decode(buf) -> !Header │    buildText/Binary    │
│ CloseCode (u16)      │  7 / 16 / 64-bit Length │    buildPing/Pong/Close│
│   1000..1011         │  FIN + MASK + MaskKey   │  Mask.apply(data, key) │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/ws/` 下全部 4 个源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/ws/opcode.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | RFC 6455 `Opcode`（`u4`：`Continue` / `Text` / `Binary` / `Close` / `Ping` / `Pong`）分类方法及 `CloseCode`（`u16`：`1000`..`1011`） |
| `lib/ws/mask.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `Mask.apply(data, key)` 原地 4 字节循环 XOR 掩码加密与解掩码 |
| `lib/ws/frame.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `FrameHeader` 编码/解码（大端序扩展长度与不完整帧校验）与 `FrameBuilder` 整帧构造器 |
| `lib/ws/index.cp` | `// Source: /data/dev/bun/src/http_jsc/websocket_client/` | `cap:ws` 统一导出与便捷命名空间 `ws` |

---

## 二、 RFC 6455 线缆格式与核心 API

### 1. RFC 6455 帧头位布局 (`FrameHeader`)

```text
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-------+-+-------------+-------------------------------+
|F|R|R|R| opcode|M| Payload len |    Extended payload length    |
|I|S|S|S|  (4)  |A|     (7)     |          (16/64 bits)         |
|N|V|V|V|       |S|             |   (if payload len==126/127)   |
| |1|2|3|       |K|             |                               |
+-+-+-+-+-------+-+-------------+-------------------------------+
|           Masking-key (4 bytes, if MASK set to 1)             |
+---------------------------------------------------------------+
```

- **自适应长度编码**：
  - `payload_len <= 125`：2 字节基础头（带掩码时共 6 字节）；
  - `126 <= payload_len <= 65535`：7-bit 字段写入 `126`，紧随 2 字节大端序 `u16`（基础 4 字节，带掩码 8 字节）；
  - `payload_len > 65535`：7-bit 字段写入 `127`，紧随 8 字节大端序 `u64`（基础 10 字节，带掩码 14 字节）。
- **边界防护**：
  - `FrameHeader.encode(out)` 在目标缓冲空间不足时返回 `error.BufferTooSmall`；
  - `FrameHeader.decode(buf)` 对长度不足 2 字节、扩展长度或 4 字节掩码键截断的报文返回 `error.IncompleteHeader`，避免越界读取。

### 2. 操作码、关闭码与掩码器

- **`Opcode`**：
  - 数据帧：`Continue (0x0)`、`Text (0x1)`、`Binary (0x2)`，`isData()` 返回 `true`；
  - 控制帧：`Close (0x8)`、`Ping (0x9)`、`Pong (0xA)`，`isControl()` 返回 `true`（判定依据 `(@intFromEnum(_) & 0x8) != 0`）。
- **`CloseCode`**：内置 `NormalClosure (1000)`、`GoingAway (1001)`、`ProtocolError (1002)`、`UnsupportedData (1003)`、`NoStatusReceived (1005)`、`AbnormalClosure (1006)`、`InvalidFramePayload (1007)`、`PolicyViolation (1008)`、`MessageTooBig (1009)`、`MissingExtension (1010)`、`InternalError (1011)`。
- **`Mask.apply(data, key)`**：原地对 `data[i] ^= key[i % 4]`，加掩码与去掩码操作完全对称，零额外内存分配。
- **`FrameBuilder`**：提供 `build`、`buildText`、`buildBinary`、`buildPing`、`buildPong` 与 `buildClose(out, code, reason, mask)`（自动将 2 字节大端 `CloseCode` 拼装于 `reason` 之前，最大载荷受 128 字节栈缓冲保护）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { Opcode, CloseCode, FrameHeader, FrameBuilder, Mask } from "cap:ws";

pub fn demoWebSocketCodec(): !void {
    // 1. 构造带掩码的客户端文本帧
    let wire: [256]u8 = undefined;
    const msg = "Hello CP WebSocket!";
    let key: [4]u8 = [0x37, 0xFA, 0x21, 0x3D];

    const wire_len = try FrameBuilder.buildText(&wire, msg, key);

    // 2. 解析帧头并还原负载
    const hdr = try FrameHeader.decode(wire[0..wire_len]);
    // hdr.fin == true, hdr.opcode == Opcode.Text, hdr.masked == true
    // hdr.payload_len == msg.len, hdr.header_len == 6

    let decoded_payload: [64]u8 = [0 as u8; 64];
    @memcpy(decoded_payload[0..hdr.payload_len], wire[hdr.header_len..wire_len]);
    Mask.apply(decoded_payload[0..hdr.payload_len], hdr.mask_key);
    // decoded_payload[0..hdr.payload_len] == "Hello CP WebSocket!"

    // 3. 构造 Ping 与 Close 控制帧
    let ctrl_buf: [64]u8 = undefined;
    const ping_len = try FrameBuilder.buildPing(&ctrl_buf, "hb", null);
    const close_len = try FrameBuilder.buildClose(&ctrl_buf, CloseCode.NormalClosure, "shutdown", null);
    * = ping_len;
    * = close_len;
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`Bun.serve({ websocket })` / `src/http_jsc/websocket_client`) | Node.js (`ws` / `undici WebSocket`) | CP 标准库 (`cap:ws`) |
| :--- | :--- | :--- | :--- |
| **操作码分类** | 内置 RFC 6455 `Opcode` | `Sender` / `Receiver` 内部常量 | `Opcode` (`isControl()` / `isData()`) |
| **关闭状态码** | WebSocket Close Code | `ws.close(1000, reason)` | `CloseCode` + `FrameBuilder.buildClose` |
| **4 字节 XOR 掩码** | SIMD / 标量 `mask` 引擎 | `bufferUtil.mask()` | `Mask.apply(data, key)` / `ws.applyMask` |
| **帧头编解码** | 内部 `WebSocketHeader` | `Receiver.getInfo()` | `FrameHeader.encode` / `FrameHeader.decode` |
| **整帧零分配构建** | `WebSocketClient.writeFrame` | `Sender.frame()` | `FrameBuilder.buildText/Binary/Ping/Pong/Close` |

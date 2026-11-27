[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# ws

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 6455 WebSocket binary frame header encoder/decoder, XOR masking, and close status codec for CP (compatible with npm ws & Rust tungstenite)

## 安装与导入

```bash
cap add ws
```

```cp
import { isControlOpcode } from "ws";
```

## 核心导出 API

- `export fn isControlOpcode(opcode: u8): bool;`
- `export fn isValidOpcode(opcode: u8): bool;`
- `export fn applyMask(payload: []u8, mask_key: [4]u8);`
- `export fn decodeHeader(data: &[]u8, out: &mut FrameHeader): i32;`
- `export fn encodeHeader(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload_len: u64, out: []u8): usize;`
- `export fn encodeFrame(fin: bool, opcode: u8, masked: bool, mask_key: [4]u8, payload: &[]u8, out: []u8): usize;`
- `export fn encodeClosePayload(code: u16, reason: string, out: []u8): usize;`
- `export fn isValidCloseCode(code: u16): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`ws@1.0.0`)，许可证：MIT

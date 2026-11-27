[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# proxy-protocol-js

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HAProxy PROXY Protocol v1 (ASCII) and v2 (binary) parser and encoder for L4 client IP preservation.

## 安装与导入

```bash
cap add proxy-protocol-js
```

```cp
import { isV1Prefix } from "proxy-protocol-js";
```

## 核心导出 API

- `export fn isV1Prefix(data: &[]u8): bool;`
- `export fn isV2Signature(data: &[]u8): bool;`
- `export fn parsePortStrict(s: string): i32;`
- `export fn parseV1(data: string, out: &mut ProxyV1Header): i32;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn appendU16(out: []u8, pos: usize, v: u16): i64;`
- `export fn encodeV1(proto: u8, src_ip: string, dst_ip: string, src_port: u16, dst_port: u16, out: []u8): usize;`
- `export fn parseV2(data: &[]u8, out: &mut ProxyV2Header): i32;`
- `export fn encodeV2Ipv4(src_ip: [4]u8, dst_ip: [4]u8, src_port: u16, dst_port: u16, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`proxy-protocol-js@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`ppp@1.0.0`)，许可证：MIT

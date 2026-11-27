[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# stomp

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation STOMP 1.2 messaging protocol frame encoder, decoder, and header escaper in pure CP

## 安装与导入

```bash
cap add stomp
```

```cp
import { strEq } from "stomp";
```

## 核心导出 API

- `export fn strEq(a: string, b: string): bool;`
- `export fn escapeHeader(input: string, out: []u8): usize;`
- `export fn unescapeHeader(input: string, out: []u8): usize;`
- `export fn encodeFrame(command: string, headers: &[]StompHeader, body: string, out: []u8): usize;`
- `export fn parseContentLength(val: string): i64;`
- `export fn decodeFrame(data: string, out_frame: &mut StompFrame): i32;`
- `export fn getHeader(frame: StompFrame, key: string): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`stomp@1.0.0`)，许可证：MIT

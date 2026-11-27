[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# tonic

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation gRPC over HTTP/2 length-prefixed frame codec, 17 status codes, and grpc-timeout parser/formatter in pure CP

## 安装与导入

```bash
cap add tonic
```

```cp
import { statusName } from "tonic";
```

## 核心导出 API

- `export fn statusName(code: u8): string;`
- `export fn isRetryableStatus(code: u8): bool;`
- `export fn encodeFrame(compressed: bool, payload: &[]u8, out: []u8): usize;`
- `export fn decodeHeader(data: &[]u8, out: &mut GrpcFrameHeader): i32;`
- `export fn parseTimeoutMicros(header: string): i64;`
- `export fn formatTimeoutMillis(ms: u64, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`tonic@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`@grpc/grpc-js@1.0.0`)，许可证：Apache-2.0

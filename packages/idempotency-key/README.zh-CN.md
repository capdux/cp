[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# idempotency-key

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation IETF Idempotency-Key HTTP header parser, validator, RFC 8941 formatter, and request fingerprint state evaluator for CP.

## 安装与导入

```bash
cap add idempotency-key
```

```cp
import { isValidKey } from "idempotency-key";
```

## 核心导出 API

- `export fn isValidKey(key: string): bool;`
- `export fn parseKeyHeader(header: string, out: []u8): usize;`
- `export fn formatKeyHeader(key: string, out: []u8): usize;`
- `export fn isUuidV4Like(key: string): bool;`
- `export fn fingerprintRequest(method: string, path: string, body: &[]u8): u64;`
- `export fn evaluateState(exists: bool, in_flight: bool, stored_fp: u64, req_fp: u64): i32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`idempotency-key@1.0.0`)，许可证：MIT

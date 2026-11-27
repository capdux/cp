[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# raw-body

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTTP request body byte-limit enforcer, Content-Length verifier, streaming chunk accumulator, and RFC 3629 UTF-8 validator for CP (clean-room parity with npm raw-body)

## 安装与导入

```bash
cap add raw-body
```

```cp
import { isValidUtf8 } from "raw-body";
```

## 核心导出 API

- `export fn isValidUtf8(data: &[]u8): bool;`
- `export fn parseByteLimit(spec: string): usize?;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`raw-body@1.0.0`)，许可证：MIT

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# requires-port

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap checker determining whether a port number is non-default for a given URL protocol

## 安装与导入

```bash
cap add requires-port
```

```cp
import { defaultPortForProtocol } from "requires-port";
```

## 核心导出 API

- `export fn defaultPortForProtocol(protocol: string): u16;`
- `export fn requiresPort(port: u16, protocol: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`requires-port@1.0.0`)（https://github.com/unshiftio/requires-port），许可证：MIT

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# resolv-conf

> 版本：`0.7.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap POSIX /etc/resolv.conf DNS resolver configuration parser

## 安装与导入

```bash
cap add resolv-conf
```

```cp
import { defaultResolvConfig } from "resolv-conf";
```

## 核心导出 API

- `export fn defaultResolvConfig(): ResolvConfig;`
- `export fn parseResolvConf(content: string): ResolvConfig;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`resolv-conf@0.7.0`)（https://github.com/tailhook/resolv-conf），许可证：MIT

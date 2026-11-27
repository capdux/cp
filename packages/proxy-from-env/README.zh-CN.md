[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# proxy-from-env

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation HTTP_PROXY / HTTPS_PROXY / ALL_PROXY / NO_PROXY URL routing and wildcard bypass matcher (clean-room MIT implementation of npm proxy-from-env, #303 / 169M wk)

## 安装与导入

```bash
cap add proxy-from-env
```

```cp
import { shouldBypassProxy } from "proxy-from-env";
```

## 核心导出 API

- `export fn shouldBypassProxy(host: string, port: u16, no_proxy: string): bool;`
- `export fn getProxyForUrl(target_url: string, env: ProxyEnv): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`proxy-from-env@1.0.0`)，许可证：MIT

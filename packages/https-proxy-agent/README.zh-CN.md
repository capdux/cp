[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# https-proxy-agent

> 版本：`7.0.6` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap HTTP/HTTPS proxy URL parser and RFC 9110 CONNECT tunnel request builder (covers http-proxy-agent)

## 安装与导入

```bash
cap add https-proxy-agent
```

```cp
import { parseProxyUrl } from "https-proxy-agent";
```

## 核心导出 API

- `export fn parseProxyUrl(url: string): ProxyEndpoint;`
- `export fn buildConnectRequest(target_host: string, target_port: u16, proxy: ProxyEndpoint, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`https-proxy-agent@7.0.6`)（https://github.com/TooTallNate/proxy-agents），许可证：MIT

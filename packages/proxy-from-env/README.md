[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:proxy-from-env` ([https://github.com/Rob--W/proxy-from-env](https://github.com/Rob--W/proxy-from-env)) | 许可：`MIT`

# `proxy-from-env`

零堆分配纯 CP `HTTP_PROXY` / `HTTPS_PROXY` / `ALL_PROXY` / `NO_PROXY` 代理路由与通配符域名绕过引擎，对标 npm **#303** `proxy-from-env`（真实周下载 **1.69 亿/周**，Axios/Got 核心依赖）。

## 核心 API

- `getProxyForUrl(target_url: string, env: ProxyEnv): string`
- `shouldBypassProxy(host: string, port: u16, no_proxy: string): bool`

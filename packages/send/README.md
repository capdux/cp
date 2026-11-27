[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# send

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:send` ([https://github.com/pillarjs/send](https://github.com/pillarjs/send))

零堆分配的纯 CP 静态文件传输规划引擎，Clean-Room MIT 实现对标 npm Top 35 包 `send` v1（周下载 1.4 亿+），跨包复用 `@cap/encodeurl`、`@cap/etag`、`@cap/range-parser` 与 `@cap/mime-types`。

## 特性

- **路径安全与点文件策略**：自动拦截 `..` 路径穿越与 NULL 字节（`403`），支持 `DOTFILES_IGNORE`（`404`）、`DOTFILES_DENY`（`403`）、`DOTFILES_ALLOW` 三档策略
- **MIME 推导**：委托 `@cap/mime-types` 推导 `content_type`，未知扩展名安全回退 `application/octet-stream`
- **ETag 与 304 协商**：委托 `@cap/etag` 生成 Stat ETag 并校验 `If-None-Match`（含 `*` 与多标签列表）
- **Range 字节区间**：委托 `@cap/range-parser` 规划 `206 Partial Content` 与越界/非法 `416`
- **安全目录重定向**：`formatRedirectUrl` 委托 `@cap/encodeurl` 编码路径、折叠多余前导斜杠并补齐尾部 `/`

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`pillarjs/send`](https://github.com/pillarjs/send)。

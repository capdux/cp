[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:fast-uri` ([https://github.com/fastify/fast-uri](https://github.com/fastify/fast-uri)) | 许可：`BSD-3-Clause`

# `@cap/fast-uri`

零堆分配纯 CP RFC 3986 URI 解析、规范化序列化、相对引用解析与等价比较库，完整对标 npm **#171** `fast-uri`（真实周下载 **2.29 亿/周**，Fastify / AJV 底层核心依赖）。

## 核心 API

- `UriComponents`：`valid`, `scheme`, `userinfo`, `host`, `port` (`-1` 表示无端口), `path`, `query`, `has_query`, `fragment`, `has_fragment`
- `parseUri(input: string): UriComponents`：支持 IPv6 `[::1]`、端口与 scheme 合法性校验、userinfo 与相对路径
- `serializeUri(c: UriComponents, out: []u8): usize`：RFC 3986 §5.3 规范重建（scheme/host 自动小写化）
- `resolveUri(base: string, rel: string, out: []u8): usize`：RFC 3986 §5.2 相对引用解析与 §5.2.4 `remove_dot_segments`
- `equalUri(a: string, b: string): bool`：RFC 3986 §6.2.2 规范化等价比较（大小写、默认端口、点段与百分号十六进制归一化）

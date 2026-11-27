[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# forwarded

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:forwarded-header-value` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `forwarded-header-value` → CP 包名 `forwarded`）

零堆分配的 RFC 7239 `Forwarded` 与 `X-Forwarded-For` 代理链安全解析引擎 — 洁净室 MIT 实现，对标 npm `forwarded` 与 `proxy-addr`。

Zero-allocation RFC 7239 `Forwarded` & `X-Forwarded-For` proxy chain parser with anti-spoofing client IP extraction and private IP detection for CP — clean-room MIT parity with npm `forwarded` and `proxy-addr`.

## 特性 / Features

- **RFC 7239 完整解析 / Full RFC 7239 support**：支持多跳逗号分隔、分号参数对、带引号字符串、IPv6 方括号 `[2001:db8::1]:4711` 与 `:port` 自动剥离。
- **防伪造真实客户端 IP 提取 / Anti-spoofing trusted-hop extraction**：从右往左跳过 `trusted_hops` 个受信代理，防止客户端伪造 `X-Forwarded-For` 头绕过限流或审计。
- **内网/私有 IP 检测 / Private IP detection**：精准识别 `10.0.0.0/8`、`172.16.0.0/12`、`192.168.0.0/16`、`127.0.0.0/8`、`169.254.0.0/16`、`::1`、`fc00::/7`、`fe80::/10` 及 IPv4-mapped `::ffff:` 私有地址。
- **零堆分配 / Zero heap allocation**：所有输出字符串直接零拷贝切片自输入缓冲区。

## 安装 / Install

```bash
cap add forwarded
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `ForwardedHop` | `{ for_addr, by_addr, host, proto }` 单跳代理信息 |
| `ForwardedChain` | 栈上最多 32 跳代理链：`count`, `hopAt(i)`, `clientIp()`, `clientProto()`, `clientHost()`, `ipAtTrustDepth(trusted_proxies)` |
| `parseForwarded(header)` | 解析 RFC 7239 `Forwarded` 请求头 |
| `parseXForwardedFor(xff, remote_addr)` | 解析 `X-Forwarded-For` 并追加 `remote_addr` |
| `extractClientIp(xff, remote_addr, trusted_hops)` | 防伪造提取真实客户端 IP（从右跳过 `trusted_hops` 个受信代理） |
| `isPrivateIp(ip)` | 判断是否为内网/私有/环回/链路本地 IPv4 或 IPv6 |

## 使用示例 / Usage

```cp
import {
    parseForwarded,
    extractClientIp,
    isPrivateIp,
} from "forwarded";

// 1. 解析 RFC 7239 Forwarded 头 / Parse RFC 7239 header
const chain = parseForwarded("for=\"[2001:db8:cafe::17]:4711\";proto=https;host=api.example.com, for=198.51.100.1;by=203.0.113.43");
const ip = chain.clientIp();       // "2001:db8:cafe::17"
const proto = chain.clientProto(); // "https"
const host = chain.clientHost();   // "api.example.com"

// 2. 防伪造提取真实客户端 IP（假设前方有 2 层受信负载均衡）/ Anti-spoofing extraction
const real_ip = extractClientIp(
    "1.2.3.4, 203.0.113.50, 10.0.0.1", // 1.2.3.4 为客户端伪造
    "10.0.0.2",                        // 直连代理 remote_addr
    2 as usize,                        // 2 个受信代理 (10.0.0.1, 10.0.0.2)
);
// real_ip => "203.0.113.50"

const internal = isPrivateIp("172.20.1.5"); // true
```

## License

MIT

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# fast-socks5

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation RFC 1928 SOCKS5 and RFC 1929 username/password authentication frame encoder and decoder for CP.

## 安装与导入

```bash
cap add fast-socks5
```

```cp
import { encodeGreeting } from "fast-socks5";
```

## 核心导出 API

- `export fn encodeGreeting(methods: &[]u8, out: []u8): usize;`
- `export fn selectMethod(greeting: &[]u8, supported: &[]u8, out_method: &mut u8): i32;`
- `export fn encodeUserPassAuth(user: string, pass: string, out: []u8): usize;`
- `export fn encodeConnectDomain(host: string, port: u16, out: []u8): usize;`
- `export fn encodeConnectIpv4(ip: [4]u8, port: u16, out: []u8): usize;`
- `export fn decodeRequest(data: &[]u8, out: &mut Socks5Request): i32;`
- `export fn encodeReplyIpv4(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`fast-socks5@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`socks@1.0.0`)，许可证：MIT

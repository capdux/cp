[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# amqplib

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation AMQP 0-9-1 / RabbitMQ URI parser (amqp/amqps, IPv6, %2F vhost, heartbeat) and password-redacting formatter in pure CP

## 安装与导入

```bash
cap add amqplib
```

```cp
import { invalidAmqp } from "amqplib";
```

## 核心导出 API

- `export fn invalidAmqp(): AmqpConfig;`
- `export fn amqpIsSpace(b: u8): bool;`
- `export fn parseU16Strict(s: string): i32;`
- `export fn parseAmqpUrl(url: string): AmqpConfig;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushU16(dst: []u8, pos: usize, val: u16): i64;`
- `export fn formatAmqpUrl(cfg: AmqpConfig, redact_password: bool, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`amqplib@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`lapin@1.0.0`)，许可证：MIT

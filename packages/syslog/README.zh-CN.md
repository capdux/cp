[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# syslog

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap RFC 5424 Syslog message parser and formatter with PRI facility/severity codec, structured data skipping, and NILVALUE handling.

## 安装与导入

```bash
cap add syslog
```

```cp
import { encodePri } from "syslog";
```

## 核心导出 API

- `export fn encodePri(facility: u8, severity: u8): u8;`
- `export fn decodeFacility(pri: u8): u8;`
- `export fn decodeSeverity(pri: u8): u8;`
- `export fn readSpToken(raw: &[]u8, pos: usize): TokenSpan;`
- `export fn parseRfc5424(raw: &[]u8): SyslogMessage;`
- `export fn appendSlice(dst: []u8, pos: usize, s: &[]u8): i64;`
- `export fn appendFieldOrNil(dst: []u8, pos: usize, field: &[]u8): i64;`
- `export fn appendU8Dec(dst: []u8, pos: usize, val: u8): i64;`
- `export fn formatRfc5424(facility: u8, severity: u8, timestamp: &[]u8, host: &[]u8, app: &[]u8, proc_id: &[]u8, msg_id: &[]u8, msg: &[]u8, dst: []u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`syslog@1.0.0`)，许可证：MIT

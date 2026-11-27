# amqplib@1.0.0 (Agent Quickref)

> Zero-allocation AMQP 0-9-1 / RabbitMQ URI parser (amqp/amqps, IPv6, %2F vhost, heartbeat) and password-redacting formatter in pure CP | License: MIT
> Import: `import { ... } from "amqplib";` | Full types: `src/index.d.cp`

## Functions
- `export fn invalidAmqp(): AmqpConfig;`
- `export fn isSpace(b: u8): bool;`
- `export fn toLowerAscii(b: u8): u8;`
- `export fn eqIgnoreCase(a: string, lower_b: string): bool;`
- `export fn parseU16Strict(s: string): i32;`
- `export fn parseAmqpUrl(url: string): AmqpConfig;`
- `export fn pushSlice(dst: []u8, pos: usize, src: string): i64;`
- `export fn pushU16(dst: []u8, pos: usize, val: u16): i64;`
- `export fn containsColon(s: string): bool;`
- `export fn formatAmqpUrl(cfg: AmqpConfig, redact_password: bool, out: []u8): usize;`

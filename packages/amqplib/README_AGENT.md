# amqplib — AI Agent Quick Ref

Zero-allocation AMQP 0-9-1 / RabbitMQ URI parser (amqp/amqps, IPv6, %2F vhost, heartbeat) and password-redacting formatter in pure CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    AmqpConfig,
    invalidAmqp,
    amqpIsSpace,
    parseU16Strict,
    parseAmqpUrl,
    pushSlice,
    pushU16,
    formatAmqpUrl,
} from "amqplib";
```

## Core Types & Constants

```cp
class AmqpConfig { tls: bool, host: string, port: u16, username: string, /* +4 fields */ }
```

## API Signatures

```cp
fn invalidAmqp(): AmqpConfig;
fn amqpIsSpace(b: u8): bool;
fn parseU16Strict(s: string): i32;
fn parseAmqpUrl(url: string): AmqpConfig;
fn pushSlice(dst: []u8, pos: usize, src: string): i64;
fn pushU16(dst: []u8, pos: usize, val: u16): i64;
fn formatAmqpUrl(cfg: AmqpConfig, redact_password: bool, out: []u8): usize;
```

## Minimal Usage

```cp
import { parseU16Strict } from "amqplib";

pub fn main() {
    let result = parseU16Strict("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"amqplib"`; install via `cap add amqplib`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.

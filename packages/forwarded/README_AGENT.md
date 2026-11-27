# forwarded — AI Agent Quick Ref

Zero-allocation RFC 7239 Forwarded & X-Forwarded-For proxy chain parser with anti-spoofing trusted-hop client IP extraction and private IP detection (forwarded / proxy-addr parity) for CP (`v1.0.0`, `MIT`)

## Import

```cp
import {
    ForwardedHop,
    ForwardedChain,
    unquote,
    stripAddr,
    parseForwardedElement,
    parseForwarded,
    parseXForwardedFor,
    extractClientIp,
} from "forwarded";
```

## Core Types & Constants

```cp
class ForwardedHop { for_addr: string, by_addr: string, host: string, proto: string } // methods: empty(...)
class ForwardedChain { hops: [MAX_HOPS]ForwardedHop, count: usize } // methods: empty(...) +5
```

## API Signatures

```cp
fn unquote(raw: string): string;
fn stripAddr(raw: string): string;
fn parseForwardedElement(elem: string): ForwardedHop;
fn parseForwarded(header: string): ForwardedChain;
fn parseXForwardedFor(xff: string, remote_addr: string): ForwardedChain;
fn extractClientIp(xff: string, remote_addr: string, trusted_hops: usize): string;
fn isPrivateIpv4(s: string): bool;
fn isPrivateIp(ip: string): bool;
```

## Minimal Usage

```cp
import { parseForwardedElement } from "forwarded";

pub fn main() {
    let result = parseForwardedElement("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"forwarded"`; install via `cap add forwarded`.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.

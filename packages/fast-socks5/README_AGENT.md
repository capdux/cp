# fast-socks5@1.0.0 (Agent Quickref)

> Zero-allocation RFC 1928 SOCKS5 and RFC 1929 username/password authentication frame encoder and decoder for CP. | License: MIT
> Import: `import { ... } from "fast-socks5";` | Full types: `src/index.d.cp`

## Functions
- `export fn encodeGreeting(methods: &[]u8, out: []u8): usize;`
- `export fn selectMethod(greeting: &[]u8, supported: &[]u8, out_method: &mut u8): i32;`
- `export fn encodeUserPassAuth(user: string, pass: string, out: []u8): usize;`
- `export fn encodeConnectDomain(host: string, port: u16, out: []u8): usize;`
- `export fn encodeConnectIpv4(ip: [4]u8, port: u16, out: []u8): usize;`
- `export fn decodeRequest(data: &[]u8, out: &mut Socks5Request): i32;`
- `export fn encodeReplyIpv4(rep: u8, bind_ip: [4]u8, bind_port: u16, out: []u8): usize;`

# `cap:socks5` — RFC 1928 & RFC 1929 SOCKS5 协议编解码库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

> **模块路径**: `lib/socks5/`  
> **导入协议**: `import { AuthMethod, SocksCommand, AddressType, ReplyCode, SocksAddr, SocksRequest, SocksReply, SocksUdpPacket, Socks5Error, ... } from "cap:net";`  
> **动态挂载**: `import cap from "cap";` -> `cap.socks5.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:socks5` 是纯 CP 实现、零 C/POSIX 外部依赖（无 `std.posix`、`std.net` 或 `@import("bun")`）的 SOCKS5（RFC 1928）及用户名/密码子协商（RFC 1929）二进制报文编解码库，专注于安全、零歧义的线协议序列化与严格边界校验：

1. **方法协商（RFC 1928 §3）**：
   - `encodeGreeting` / `decodeGreeting`：校验版本号 `0x05`、方法数量范围 `1..255`，拒绝截断与多余尾随字节。
   - `encodeMethodChoice` / `decodeMethodChoice`：编码与解码服务端选定的 `AuthMethod`。
2. **用户名/密码认证（RFC 1929）**：
   - `encodeUserPassAuth` / `decodeUserPassAuth`：校验子协商版本号 `0x01` 及用户名/密码长度 `1..255`（零拷贝返回切片）。
   - `encodeAuthStatus` / `decodeAuthStatus`：编码与解码认证状态响应（`0x00` 表示成功）。
3. **请求与回复（RFC 1928 §4 & §6）**：
   - 完整支持 `AddressType.Ipv4`（4 字节）、`AddressType.Domain`（1 字节长度 + 1..255 字节 FQDN）、`AddressType.Ipv6`（16 字节）及大端序 `u16` 端口。
   - 严格拦截非法版本号、非法命令/回复码/地址类型、超 255 字节域名（`DomainTooLong`）、截断报文（`UnexpectedEof`）与多余尾随字节（`TrailingBytes`）。
4. **UDP 转发头（RFC 1928 §7）**：
   - `encodeUdpPacket` / `decodeUdpPacket`：支持 `RSV(2) | FRAG(1) | ATYP(1) | DST.ADDR | DST.PORT(2) | DATA` 完整往返编解码。

---

## 二、 核心 API 速查表 (`lib/socks5/socks5.cp`)

| 符号 / 函数 | 签名与说明 |
| :--- | :--- |
| `Socks5Error` | `InvalidVersion`, `UnexpectedEof`, `InvalidMethodCount`, `InvalidAddressType`, `InvalidCommand`, `InvalidReplyCode`, `DomainTooLong`, `CredentialsTooLong`, `TrailingBytes`, `OutOfMemory` |
| `AuthMethod` | `NoAuth = 0x00`, `Gssapi = 0x01`, `UserPass = 0x02`, `NoAcceptable = 0xFF` |
| `SocksCommand` | `Connect = 0x01`, `Bind = 0x02`, `UdpAssociate = 0x03` |
| `AddressType` | `Ipv4 = 0x01`, `Domain = 0x03`, `Ipv6 = 0x04` |
| `ReplyCode` | `Succeeded = 0x00` .. `AddressTypeNotSupported = 0x08` |
| `SocksAddr` | Tagged union：`ipv4: [4]u8`, `domain: []const u8`, `ipv6: [16]u8` |
| `UserPassCredentials` | `{ username: []const u8, password: []const u8 }` |
| `SocksRequest` | `{ cmd: SocksCommand, addr: SocksAddr, port: u16 }` |
| `SocksReply` | `{ reply: ReplyCode, addr: SocksAddr, port: u16 }` |
| `SocksUdpPacket` | `{ frag: u8, addr: SocksAddr, port: u16, payload: []const u8 }` |
| `encodeGreeting(allocator, methods)` | 编码客户端 Greeting 报文（调用方负责 `allocator.free`） |
| `decodeGreeting(bytes)` | 解码客户端 Greeting 报文，返回方法字节切片 |
| `encodeMethodChoice(method)` | 编码服务端方法选择（`[2]u8`） |
| `decodeMethodChoice(bytes)` | 解码服务端方法选择，返回 `AuthMethod` |
| `encodeUserPassAuth(allocator, user, pass)` | 编码 RFC 1929 账密认证报文（调用方负责 `allocator.free`） |
| `decodeUserPassAuth(bytes)` | 解码 RFC 1929 账密认证报文，返回 `UserPassCredentials` |
| `encodeAuthStatus(status)` | 编码 RFC 1929 认证状态响应（`[2]u8`） |
| `decodeAuthStatus(bytes)` | 解码 RFC 1929 认证状态响应，返回 `u8` |
| `encodeRequest(allocator, req)` | 编码 `SocksRequest`（调用方负责 `allocator.free`） |
| `decodeRequest(bytes)` | 解码 `SocksRequest` |
| `encodeReply(allocator, rep)` | 编码 `SocksReply`（调用方负责 `allocator.free`） |
| `decodeReply(bytes)` | 解码 `SocksReply` |
| `encodeUdpPacket(allocator, pkt)` | 编码 `SocksUdpPacket`（调用方负责 `allocator.free`） |
| `decodeUdpPacket(bytes)` | 解码 `SocksUdpPacket` |

---

## 三、 使用示例

```typescript
const std = @import("std");
import {
    AuthMethod,
    SocksCommand,
    ReplyCode,
    SocksAddr,
    SocksRequest,
    encodeGreeting,
    decodeGreeting,
    encodeRequest,
    decodeRequest,
} from "cap:net";

fn demo(alloc: std.mem.Allocator): !void {
    const methods: [2]AuthMethod = [AuthMethod.NoAuth, AuthMethod.UserPass];
    const greeting = encodeGreeting(alloc, &methods)?;
    defer alloc.free(greeting);

    const decoded_methods = decodeGreeting(greeting)?;
    _ = decoded_methods;

    const req = SocksRequest{
        cmd: SocksCommand.Connect,
        addr: SocksAddr.domain("api.cp.dev"),
        port: 443,
    };
    const wire = encodeRequest(alloc, req)?;
    defer alloc.free(wire);

    const parsed = decodeRequest(wire)?;
    _ = parsed;
}
```

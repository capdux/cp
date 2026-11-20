# `cap:ssh_wire` 标准库参考文档

## 1. 概述

`cap:ssh_wire`（亦可通过 `cap.ssh_wire` 访问）提供纯 CP 实现的工业级 SSH-2 Wire 二进制编解码、OpenSSH 公私钥格式解析及 `SSHSIG` 签名/验签能力：

- **RFC 4251 Wire 编解码**：支持大端 `u32`、`string`、最小化二补码 `mpint` 及逗号分隔 `name-list`。
- **OpenSSH 公钥与指纹**：支持 `ssh-ed25519` 与 `ecdsa-sha2-nistp256` 公钥行解析、格式化及 `SHA256:<base64_no_pad>` 指纹计算。
- **OpenSSH 私钥 (`openssh-key-v1`)**：支持未加密（`none` 加密与 KDF）Ed25519 私钥的 PEM 解析与编码，严格校验 `openssh-key-v1\0` 魔数与 `checkint1 == checkint2`。
- **`SSHSIG` 签名与验签**：严格遵循 OpenSSH `SSHSIG` 规范（`SSHSIG` 魔数、`namespace`、`sha512` 载荷哈希及 `-----BEGIN SSH SIGNATURE-----` PEM 封装）。

## 2. 核心类型

| 类型 | 说明 |
| --- | --- |
| `SshError` | SSH 编解码、密钥解析及签名验签统一错误集 |
| `SshReader` | 零拷贝 SSH-2 Wire 二进制顺序读取器 |
| `SshWriter` | 基于 `std.ArrayList(u8)` 的 SSH-2 Wire 二进制构建器 |
| `SshKeyType` | 公钥算法枚举：`Ed25519`、`EcdsaP256` |
| `SshPublicKey` | 已解析的 OpenSSH 公钥结构（含原始 Wire Blob 与注释） |
| `OpenSshPrivateKey` | 已解析的 `openssh-key-v1` Ed25519 私钥结构 |

## 3. API 速查

### Wire 编解码
- `SshReader.init(data: []const u8) -> SshReader`
- `reader.readU32(*_) -> SshError!u32`
- `reader.readString(*_) -> SshError![]const u8`
- `reader.readMpint(*_) -> SshError![]const u8`
- `reader.readNameList(*_, alloc: Allocator) -> SshError![][]const u8`
- `SshWriter.init(alloc: Allocator) -> SshWriter` / `writer.deinit(*_)`
- `writer.writeU32(*_, val: u32) -> SshError!void`
- `writer.writeString(*_, data: []const u8) -> SshError!void`
- `writer.writeMpint(*_, bytes: []const u8) -> SshError!void`
- `writer.writeSignedMpint(*_, bytes: []const u8) -> SshError!void`
- `writer.writeNameList(*_, names: []const []const u8) -> SshError!void`
- `writer.toOwnedSlice(*_) -> SshError![]u8`

### OpenSSH 公私钥与指纹
- `parsePublicKeyLine(alloc: Allocator, line: []const u8) -> SshError!SshPublicKey`
- `formatPublicKeyLine(alloc: Allocator, key: *const SshPublicKey) -> SshError![]u8`
- `computeSha256Fingerprint(alloc: Allocator, raw_blob: []const u8) -> SshError![]u8`
- `parseOpenSshPrivateKey(alloc: Allocator, pem_text: []const u8) -> SshError!OpenSshPrivateKey`
- `encodeOpenSshPrivateKeyEd25519(alloc: Allocator, secret_64: [64]u8, comment: []const u8, checkint: u32) -> SshError![]u8`

### `SSHSIG` 签名与验签
- `signSshSigEd25519(alloc: Allocator, secret_64: [64]u8, namespace: []const u8, message: []const u8) -> SshError![]u8`
- `verifySshSigEd25519(alloc: Allocator, pub_32: [32]u8, namespace: []const u8, message: []const u8, armoring_text: []const u8) -> SshError!void`

## 4. 示例

```cp
import { SshWriter, SshReader, signSshSigEd25519, verifySshSigEd25519 } from "cap:ssh_wire";
const std = @import("std");

pub fn demo(alloc: std.mem.Allocator, secret_64: [64]u8, pub_32: [32]u8) !void {
    const sig_pem = try signSshSigEd25519(alloc, secret_64, "file", "hello cp sshsig");
    defer alloc.free(sig_pem);

    try verifySshSigEd25519(alloc, pub_32, "file", "hello cp sshsig", sig_pem);
}
```

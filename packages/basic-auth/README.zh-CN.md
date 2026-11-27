[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# basic-auth

纯 CP 实现的零堆分配 RFC 7617 HTTP Basic Authentication 解析、编码、恒定时间凭据校验与 `WWW-Authenticate` 质询头格式化库。

- **源包对齐**：对标 `npm` `basic-auth`（jshttp/basic-auth）与 `crates.io` `http-auth-basic`（MIT）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，使用调用方栈缓冲区 `[]u8` 与 `cap:bin.ctEqual` 恒定时间比对。

## 安装与导入

```bash
cap add basic-auth
```

```cp
import {
    BasicCredentials,
    isWs,
    b64EncChar,
    b64DecVal,
    b64Encode,
    b64Decode,
    decodeHeader,
    encodeHeader,
    constantTimeEq,
    verifyHeader,
    appendSlice,
    formatChallenge,
} from "basic-auth";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `BasicCredentials` | `class BasicCredentials { user_len: usize, pass_len: usize, valid: bool }` | `decodeHeader` 返回结果，记录写入 `out_user`/`out_pass` 的字节长度及合法性。 |
| `decodeHeader` | `fn decodeHeader(header: string, out_user: []u8, out_pass: []u8): BasicCredentials` | 解析 `"Basic <base64>"`（忽略方案名大小写），将 `user:pass` 解码写入调用方缓冲区。 |
| `encodeHeader` | `fn encodeHeader(user: string, pass: string, out: []u8): usize` | 编码 `"Basic <base64(user:pass)>"` 写入 `out`，返回写入字节数（缓冲不足返回 `0`）。 |
| `verifyHeader` | `fn verifyHeader(header: string, expected_user: string, expected_pass: string): bool` | 解码请求头并在恒定时间内校验用户名与密码。 |
| `formatChallenge` | `fn formatChallenge(realm: string, utf8: bool, out: []u8): usize` | 格式化 RFC 7617 `Basic realm="..."`（`utf8 == true` 时附加 `, charset="UTF-8"`）写入 `out`。 |
| `constantTimeEq` | `fn constantTimeEq(a: &[]u8, b: &[]u8): bool` | 恒定时间字节切片相等比较，防止时序侧信道泄露。 |
| `b64Encode` / `b64Decode` | `fn b64Encode(src: &[]u8, dst: []u8): i64` / `fn b64Decode(src: &[]u8, dst: []u8): i64` | 标准 RFC 4648 Base64 编解码，返回写入字节数或 `-1`。 |
| `isWs` / `b64EncChar` / `b64DecVal` / `appendSlice` | 底层辅助函数 | ASCII 空白判断、Base64 字符查表与安全追加写入（溢出返回 `-1`）。 |

## 使用示例

### 1. 编码与恒定时间校验 `Authorization` 头

```cp
import { encodeHeader, verifyHeader } from "basic-auth";

fn authDemo(): void {
    let mut hdr_buf: [128]u8 = [0; 128];
    const n = encodeHeader("alice", "s3cr3t", hdr_buf[0..]);
    // hdr_buf[0..n] == "Basic YWxpY2U6czNjcjN0"

    const ok = verifyHeader("Basic YWxpY2U6czNjcjN0", "alice", "s3cr3t"); // true
}
```

### 2. 将凭据解码到栈缓冲区

```cp
import { decodeHeader } from "basic-auth";

fn decodeDemo(auth_header: string): void {
    let mut user_buf: [64]u8 = [0; 64];
    let mut pass_buf: [64]u8 = [0; 64];
    const creds = decodeHeader(auth_header, user_buf[0..], pass_buf[0..]);
    if (creds.valid) {
        // user_buf[0..creds.user_len], pass_buf[0..creds.pass_len]
    }
}
```

### 3. 格式化 `WWW-Authenticate` 质询响应头

```cp
import { formatChallenge } from "basic-auth";

fn challengeDemo(): void {
    let mut buf: [128]u8 = [0; 128];
    const n = formatChallenge("Admin Area", true, buf[0..]);
    // buf[0..n] == `Basic realm="Admin Area", charset="UTF-8"`
}
```

## 与源包差异与安全亮点

- **内置恒定时间校验**：`verifyHeader` 与 `constantTimeEq` 基于 `cap:bin.ctEqual`，避免密码校验中的时序泄露。
- **完整 RFC 7617 双向支持**：除解析外还提供 `encodeHeader` 与 `formatChallenge`（支持 `charset="UTF-8"`）。
- **零堆分配**：密码仅短暂驻留于调用方栈缓冲区，便于校验后立即清零，不留堆内存痕迹。

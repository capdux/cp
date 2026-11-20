# `cap:pem` — RFC 7468 PEM 证书链与密钥块编解码库

> **模块路径**: `lib/pem/`  
> **导入协议**: `import { PemBlock, PemHeader, PemError, parse, parseFirst, parseWithMaxBlocks, encode, encodeWithHeaders, encodeChain, freeBlocks } from "cap:pem";`  
> **动态挂载**: `import cap from "cap";` -> `cap.pem.*` 编译期按需惰性挂载

---

## 一、 架构概述

`cap:pem` 是纯 CP 实现、零 C 外部依赖（无 `std.posix` / `@import("bun")`）的 RFC 7468 PEM（Privacy-Enhanced Mail）文本封装编解码器，适用于 X.509 证书链、PKCS#8 / RSA / EC 私钥及传统 RFC 1421 加密私钥块：

1. **多块证书链与注释容错**：
   - `parse` / `parseWithMaxBlocks` 自动跳过块前、块间注释文本与空行，同时兼容 `\r\n` 与 `\n` 换行符。
   - `parseFirst` 仅提取并解码首个 PEM 块（无块时返回 `null`）。
2. **RFC 1421 前导头字段支持**：
   - 支持 `Proc-Type: 4,ENCRYPTED`、`DEK-Info: AES-128-CBC,...` 等可选前导 `Key: Value` 头字段解析与编码，并提供 `block.header(name)` 大小写不敏感查询。
3. **严格边界校验与资源耗尽防护**：
   - 严格校验 `-----BEGIN <LABEL>-----` 与 `-----END <LABEL>-----` 标签一致性（`MismatchedEndLabel`）、缺失结束标记（`MissingEndMarker`）、空载荷（`EmptyPayload`）、非法标签（`InvalidLabel`）以及严格 Base64 字符/填充校验（`InvalidBase64`）。
   - `parseWithMaxBlocks` 支持显式配置最大块数上限，超出时返回 `PemError.TooManyBlocks`。
4. **RFC 7468 64 列折行编码**：
   - `encode`、`encodeWithHeaders` 与 `encodeChain` 均严格按 64 字符固定宽度折行输出标准 Base64 与 `-----BEGIN/END <LABEL>-----` 边界。

---

## 二、 核心 API 速查表 (`lib/pem/pem.cp`)

| 符号 / 方法 | 签名与说明 |
| :--- | :--- |
| `PemError` | `InvalidLabel`, `MismatchedEndLabel`, `MissingEndMarker`, `InvalidBase64`, `EmptyPayload`, `TooManyBlocks`, `OutOfMemory` |
| `PemHeader` | `{ name: []const u8, value: []const u8 }` |
| `PemBlock` | `{ label: []const u8, headers: []PemHeader, bytes: []const u8 }`；方法：`deinit(*_, allocator)`、`header(name)` |
| `parse(allocator, pem_text)` | 解析全部 PEM 块（默认上限 64 块），返回 `PemError![]PemBlock` |
| `parseWithMaxBlocks(allocator, pem_text, max_blocks)` | 带最大块数上限防护的 PEM 链解析，超出返回 `PemError.TooManyBlocks` |
| `parseFirst(allocator, pem_text)` | 仅解析首个 PEM 块，无块时返回 `null` |
| `encode(allocator, label, bytes)` | 将原始字节编码为 64 列折行的标准 PEM 块 |
| `encodeWithHeaders(allocator, label, headers, bytes)` | 编码带 RFC 1421 `Key: Value` 头字段的 PEM 块 |
| `encodeChain(allocator, blocks)` | 将多个 `PemBlock` 连续编码为证书链 PEM 文本 |
| `freeBlocks(allocator, blocks)` | 释放 `parse` / `parseWithMaxBlocks` 返回的全部 `PemBlock` 及外层切片 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { parse, encode, encodeWithHeaders, freeBlocks, PemHeader } from "cap:pem";

fn demo(alloc: std.mem.Allocator): !void {
    const der = "hello-der-certificate-payload";
    const pem_text = encode(alloc, "CERTIFICATE", der)?;
    defer alloc.free(pem_text);

    const blocks = parse(alloc, pem_text)?;
    defer freeBlocks(alloc, blocks);

    const headers: [2]PemHeader = [
        PemHeader{ name: "Proc-Type", value: "4,ENCRYPTED" },
        PemHeader{ name: "DEK-Info", value: "AES-128-CBC,0123456789ABCDEF" },
    ];
    const key_pem = encodeWithHeaders(alloc, "RSA PRIVATE KEY", &headers, der)?;
    defer alloc.free(key_pem);
}
```

# `cap:grpc_wire` — gRPC over HTTP/2 线协议标准库

## 概述

`cap:grpc_wire`（`lib/grpc_wire/`，同时支持 `import cap from "cap"` 动态挂载 `cap.grpc_wire`）提供纯 CP 实现的工业级 gRPC over HTTP/2 线协议编解码工具集，涵盖 Length-Prefixed Message 5 字节帧编解码、流式分帧器、`grpc-timeout` / `grpc-message` / `-bin` 头编解码、17 种标准 gRPC 状态码以及 `:path` 服务方法路由解析。

## 核心类型

- **`GrpcStatus`**：`u32` 枚举，完整覆盖 gRPC 规范 `OK (0)` 至 `UNAUTHENTICATED (16)` 共 17 种标准状态码。
- **`GrpcError`**：包含 `status: GrpcStatus` 与 `message: []const u8` 的结构化错误描述。
- **`ServiceMethodPath`**：包含 `package_service: []const u8` 与 `method: []const u8` 的路由解析结果。
- **`GrpcFrame`**：包含 `compressed: bool` 与 `payload: []u8`，提供 `deinit(alloc)` 释放载荷内存。
- **`GrpcStreamFramer`**：增量流式分帧器，支持任意字节切片跨 chunk 拆包连续消息，内置 `compressed_flag <= 1` 校验与 `max_message_bytes`（默认 `DEFAULT_MAX_MESSAGE_BYTES = 4 MiB`）防内存炸弹拦截。

## API 速查

| 符号 | 签名概要 | 说明 |
| --- | --- | --- |
| `statusName` | `(status: GrpcStatus): []const u8` | 返回状态码标准名称字符串 |
| `statusFromCode` | `(code: u32): GrpcStatus?` | 数字状态码转 `GrpcStatus`（越界返回 `null`） |
| `parseServiceMethodPath` | `(path: []const u8): !ServiceMethodPath` | 解析 `/pkg.Svc/Method`，拦截非法斜杠 |
| `formatServiceMethodPath` | `(alloc, pkg_svc, method): ![]u8` | 格式化合法的 `:path` 路由 |
| `parseGrpcTimeoutNs` | `(header_val: []const u8): !u64` | 解析 `H`/`M`/`S`/`m`/`u`/`n`（最多 8 位数字）为纳秒 |
| `formatGrpcTimeout` | `(alloc, ns: u64): ![]u8` | 将纳秒格式化为不超过 8 位数字的 `grpc-timeout` 字符串 |
| `percentEncodeGrpcMessage` | `(alloc, msg: []const u8): ![]u8` | 按 gRPC 规范转义非 `0x20..0x7E` 及 `%` 为 `%XX` |
| `percentDecodeGrpcMessage` | `(alloc, encoded: []const u8): ![]u8` | 解码 `%XX` 百分号编码并校验合法性 |
| `encodeBinHeader` | `(alloc, raw: []const u8): ![]u8` | Base64 编码 `-bin` 二进制头 |
| `decodeBinHeader` | `(alloc, b64: []const u8): ![]u8` | Base64 解码 `-bin` 头（兼容有无 `=` 填充）并拦截畸形输入 |
| `encodeGrpcFrame` | `(alloc, compressed: bool, payload: []const u8): ![]u8` | 编码 5 字节头 + payload |
| `decodeGrpcFrame` | `(alloc, data: []const u8, max_bytes: usize): !GrpcFrame` | 单帧解码与上限校验 |
| `GrpcStreamFramer` | `init(max_bytes)` / `push(*_, alloc, chunk)` / `nextFrame(*_, alloc)` / `deinit(*_, alloc)` | 跨 chunk 增量流式拆包 |

## 示例

```cp
import { encodeGrpcFrame, GrpcStreamFramer, DEFAULT_MAX_MESSAGE_BYTES } from "cap:grpc_wire";

const wire = try encodeGrpcFrame(alloc, false, "hello-grpc");
defer alloc.free(wire);

let framer = GrpcStreamFramer.init(DEFAULT_MAX_MESSAGE_BYTES);
defer framer.deinit(alloc);

try framer.push(alloc, wire[0..3]);
try framer.push(alloc, wire[3..]);
if (try framer.nextFrame(alloc)) |*frame| {
    defer frame.deinit(alloc);
    // 使用 frame.payload
}
```

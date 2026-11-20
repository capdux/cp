# `cap:msgpack_rpc` — MessagePack-RPC 协议帧与流式多路复用解码标准库

## 1. 概述
`cap:msgpack_rpc`（动态挂载 `cap.msgpack_rpc`）是纯 CP 实现的 MessagePack-RPC 协议编解码与增量流式拆帧标准库。完整支持 Request (`[0, msgid, method, params]`)、Response (`[1, msgid, error, result]`)、Notification (`[2, method, params]`) 三类消息，内置 `MAX_RPC_DEPTH = 32` 与 `max_msg_bytes` 防资源耗尽保护。

## 2. 核心类型
- `RpcError`：`Incomplete`、`UnexpectedEof`、`InvalidFormat`、`InvalidMsgType`、`InvalidFrame`、`MaxDepthExceeded`、`MessageTooLarge`、`OutOfMemory`
- `RpcValue`：`nil`/`bool_val`/`int_val`/`uint_val`/`float_val`/`str_val`/`bin_val`/`array_val`/`map_val`
- `RpcMessage`：`request`/`response`/`notification`
- `RpcStreamDecoder`：支持任意切片边界半包黏包增量拆帧

## 3. API 速查
- `encodeRpcMessage(alloc, msg)` -> `[]u8`
- `decodeRpcMessage(alloc, bytes)` -> `DecodedRpcMessage`
- `RpcStreamDecoder.init(alloc, max_msg_bytes)` / `feed(chunk)` / `next()` / `deinit()`

## 4. 示例
```cp
import { RpcMessage, RpcRequest, RpcValue, encodeRpcMessage, RpcStreamDecoder } from "cap:msgpack_rpc";
```

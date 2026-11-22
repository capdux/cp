# `cap:wireguard` (`cap.wireguard`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 概述

`cap:wireguard` 提供纯 CP 实现的工业级 WireGuard `Noise_IKpsk2` 密码学握手原语、TAI64N 单调时间戳与 RFC 6479 2048-bit 防重放滑动窗口标准库。全模块采用零堆分配定长缓冲区设计，底层密码学原语复用 `X25519`、`BLAKE2s-256` / `BLAKE2s-128` 与 `ChaCha20-Poly1305`，并通过 `std.crypto.timing_safe.eql` 实现常量时间 MAC1 与公钥比对。

## 核心类型

- **`WgError`**：协议与密码学错误枚举（`InvalidLength`、`InvalidMessageType`、`InvalidMac1`、`InvalidMac2`、`ReplayDetected`、`WindowOutOfRange`、`StaleTimestamp`、`DecryptFailed`、`InvalidKey`、`InvalidState`）。
- **`Tai64n`**：12 字节大端 WireGuard TAI64N 时间戳（`sec: u64`, `nsec: u32`），支持 `fromUnixNs`、`encode`、`decode` 与 `compare`。
- **`ReplayWindow`**：零堆分配 RFC 6479 滑动窗口（`[32]u64` 位图，有效窗口 1984 位），支持 `init` 与 `checkAndUpdate`。
- **`WgKeyPair` / `WgSessionKeys`**：X25519 静态/临时密钥对及握手派生出的双向传输密钥（`send_key` / `recv_key`、`local_index` / `remote_index`）。
- **`HandshakeInitiation` (148B) / `HandshakeResponse` (92B) / `TransportPacket`**：Type 1 / Type 2 / Type 4 报文结构与编解码。
- **`NoiseInitiatorState` / `InitiationResult` / `ResponderResult`**：握手状态机中间态与结果封装。

## API 速查

| 符号 | 签名摘要 | 说明 |
| :--- | :--- | :--- |
| `Tai64n.fromUnixNs` | `(unix_sec: i64, unix_nsec: u32) -> Tai64n` | 从 Unix 秒/纳秒构造 TAI64N 时间戳 |
| `Tai64n.encode` / `decode` | `() -> [12]u8` / `(data: []const u8) -> !Tai64n` | 12 字节大端编解码 |
| `Tai64n.compare` | `(other: Tai64n) -> i32` | 时间戳单调比较（1 / 0 / -1） |
| `ReplayWindow.init` | `() -> ReplayWindow` | 初始化零堆分配滑动窗口 |
| `ReplayWindow.checkAndUpdate` | `(*_, counter: u64) -> !void` | RFC 6479 序号防重放检查与窗口推进 |
| `wgKdf1` / `wgKdf2` / `wgKdf3` | `(key: *const [32]u8, input: []const u8) -> ...` | 基于 HMAC-BLAKE2s256 的 1/2/3 路密钥派生 |
| `blake2sMac16` | `(key: *const [32]u8, input: []const u8) -> [16]u8` | Keyed BLAKE2s-128 MAC1/MAC2 计算 |
| `createInitiation` | `(local_static, remote_static_pub, psk, sender_index, timestamp) -> !InitiationResult` | 生成 148 字节 Type 1 握手请求 |
| `consumeInitiationAndCreateResponse` | `(responder_static, expected_peer_static, psk, responder_index, last_timestamp, msg) -> !ResponderResult` | 校验 MAC1/TAI64N 并生成 92 字节 Type 2 响应 |
| `consumeResponse` | `(local_static, state, msg) -> !WgSessionKeys` | 校验响应并派生 Initiator 传输密钥 |
| `encryptTransport` | `(keys, counter, plaintext, out_buf) -> !usize` | 封装并 AEAD 加密 Type 4 传输帧 |
| `decryptTransport` | `(keys, window, packet, out_plaintext) -> !TransportPacket` | AEAD 解密 Type 4 帧并更新防重放窗口 |

## 示例

```cp
import { WgKeyPair, Tai64n, ReplayWindow, createInitiation, consumeInitiationAndCreateResponse, consumeResponse, encryptTransport, decryptTransport } from "cap:net";

const init_kp = try WgKeyPair.generate();
const resp_kp = try WgKeyPair.generate();
const psk: [32]u8 = [7; 32];
let last_ts: Tai64n? = null;

const init_res = try createInitiation(init_kp, resp_kp.public_key, psk, 1001, Tai64n.fromUnixNs(1700000000, 123));
const resp_res = try consumeInitiationAndCreateResponse(resp_kp, init_kp.public_key, psk, 2002, &last_ts, init_res.msg);
const init_keys = try consumeResponse(init_kp, init_res.state, resp_res.msg);

let pkt: [128]u8 = [0; 128];
const pkt_len = try encryptTransport(init_keys, 0, "hello wg", &pkt);

let win = ReplayWindow.init();
let plain: [64]u8 = [0; 64];
const tp = try decryptTransport(resp_res.keys, &win, pkt[0..pkt_len], &plain);
```

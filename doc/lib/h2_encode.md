# `cap:h2_encode` — 零堆分配 HTTP/2 出站请求与流控帧编码库

`cap:h2_encode` 吸收自 Bun 上游 `src/http/h2_client/encode.cp`，剥离了底层 I/O 与分配器耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 RFC 9113 HTTP/2 出站请求头过滤、RFC 7541 HPACK 动态表大小更新编码、客户端连接序言编排、`HEADERS`/`CONTINUATION` 自动分片以及双窗口流控 `DATA` 帧编码。

## 核心特性

- **100% 零堆分配**：全部帧头、序言、分片块及小写化头部名均直接写入调用方提供的 `[]u8` 缓冲。
- **RFC 9113 §8.2.2 请求头分类与过滤**：自动剔除 HTTP/1.x hop-by-hop 头（`Connection`、`Keep-Alive`、`Proxy-Connection`、`Transfer-Encoding`、`Upgrade`）及 `Host`；`TE` 仅允许 `"trailers"`；在 `Expect: 100-continue` 场景下自动抑制 `Content-Length`；标记 `Authorization`/`Cookie`/`Set-Cookie` 为 HPACK `never-indexed` 敏感头。
- **RFC 7541 §6.3 HPACK 动态表大小更新编码**：精确实现 5-bit 前缀 `0x20` 变长整数编码。
- **客户端连接序言（`encodeClientPreface`）**：单次生成 24B Preface + 3 项 SETTINGS（`ENABLE_PUSH=0`, `INITIAL_WINDOW_SIZE`, `MAX_HEADER_LIST_SIZE`）+ 可选连接级 `WINDOW_UPDATE`。
- **`HEADERS` + `CONTINUATION` 分片（`encodeHeaderBlockFrames`）**：超长 HPACK 头部块按 `max_frame_size` 自动切分，仅首帧置 `END_STREAM`（若开启）、仅末帧置 `END_HEADERS`。
- **双窗口流控 `DATA` 编码（`encodeDataWindowed`）**：受 `min(stream_window, conn_window)`、`max_frame_size` 和 `send_cap` 三重约束，精确扣减流/连接窗口，仅最终字节发送完毕时置 `END_STREAM`。

---

## 导出清单

### 常量、枚举与结构体

- 协议常量：`H2_CLIENT_PREFACE`、`H2_FRAME_HEADER_LEN`、`H2_DEFAULT_WINDOW_SIZE`、`H2_DEFAULT_MAX_FRAME_SIZE`、`H2_MAX_FRAME_SIZE_LIMIT`
- 帧类型与标志：`H2_FRAME_DATA`、`H2_FRAME_HEADERS`、`H2_FRAME_SETTINGS`、`H2_FRAME_WINDOW_UPDATE`、`H2_FRAME_CONTINUATION`、`H2_FLAG_END_STREAM`、`H2_FLAG_END_HEADERS`
- Settings ID：`H2_SETTINGS_ENABLE_PUSH`、`H2_SETTINGS_INITIAL_WINDOW_SIZE`、`H2_SETTINGS_MAX_HEADER_LIST_SIZE`
- `RequestHeaderAction`：`Forward | ForwardNeverIndex | Drop | Host | Te | ContentLength | Expect`
- `HeaderBlockEncodeResult`：`{ bytes_written, frame_count }`
- `DataWindowState`：`{ stream_window, conn_window }`
- `DataWriteResult`：`{ bytes_written, payload_consumed, frame_count, sent_end_stream }`

### 核心函数

| 函数签名 | 说明 |
| :--- | :--- |
| `classifyRequestHeader(name): RequestHeaderAction` | 大小写不敏感分类 HTTP/1.x 请求头 |
| `shouldSkipRequestHeader(name, value, has_expect_continue): bool` | 判断是否应按 RFC 9113 跳过该请求头 |
| `isSensitiveHeader(name): bool` | 判断是否应使用 HPACK never-indexed |
| `lowercaseHeaderName(name, out_buf): usize?` | 零分配 ASCII 小写化头部名 |
| `encodeHpackTableSizeUpdate(value, out_buf): usize?` | RFC 7541 §6.3 动态表大小更新编码 |
| `encodeFrameHeader(payload_len, frame_type, flags, stream_id, out_buf): usize?` | 9 字节大端 HTTP/2 帧头编码 |
| `encodeClientPreface(local_initial_window, local_max_header_list, out_buf): usize?` | 客户端序言 + SETTINGS + 可选 WINDOW_UPDATE |
| `encodeHeaderBlockFrames(stream_id, block, max_frame_size, end_stream, out_buf): HeaderBlockEncodeResult?` | `HEADERS` + `CONTINUATION` 自动分片编码 |
| `encodeDataWindowed(stream_id, data, max_frame_size, end_stream, send_cap, win, out_buf): DataWriteResult?` | 双窗口流控 `DATA` 分片编码 |

---

## 使用示例

```typescript
import std from "std";
import {
    DataWindowState,
    encodeClientPreface,
    encodeHeaderBlockFrames,
    encodeDataWindowed,
    shouldSkipRequestHeader,
} from "cap:h2_encode";

fn h2EncodeDemo() {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    let out: []u8 = buf[0..];
    const pref_len = encodeClientPreface(262144, 65536, &out);
    * = pref_len;

    const skip_conn = shouldSkipRequestHeader("Connection", "keep-alive", false);
    * = skip_conn;

    const hdr_block = "hpack-bytes";
    const hres = encodeHeaderBlockFrames(1, hdr_block, 16384, false, &out);
    * = hres;

    let win = DataWindowState { stream_window: 65535, conn_window: 65535 };
    const dres = encodeDataWindowed(1, "hello h2", 16384, true, 65535, &win, &out);
    * = dres;
}
```

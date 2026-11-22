# `cap:h2stream` — 零堆分配 HTTP/2 流状态机与流控窗口库

`cap:h2stream` 吸收自 Bun 上游 HTTP/2 多路复用流实现（`src/http/h2_client/Stream.cp`），剥离了会话指针与堆缓冲耦合，提供 100% 纯 CP、零堆分配（zero-alloc）的 RFC 9113 §5.1 流生命周期状态机、§6.9 双向流量控制窗口管理、§6.4 幂等 `RST_STREAM` 帧编码以及背压阈值触发的 `WINDOW_UPDATE` 自动补窗器。

## 核心特性

- **100% 零堆分配**：`H2Stream` 为纯值类型状态机，帧编码直接写入调用方 13 字节 `[]u8` 切片。
- **RFC 9113 §5.1 流状态转移**：精确跟踪 `Open -> HalfClosedLocal / HalfClosedRemote -> Closed`、`100 Continue` 解除发送阻塞及幂等 `reset`（防重复发送 `RST_STREAM`）。
- **RFC 9113 §6.9 双向流控与溢出防护**：
  - 发送窗口：`consumeSendWindow`、`applySendWindowUpdate`（严格拦截 0 增量与 `> 2^31 - 1` 溢出）及 `applySettingsInitialWindowDelta`（合法支持因对端调小 `SETTINGS_INITIAL_WINDOW_SIZE` 产生的负窗口）；
  - 接收窗口：`recordReceivedData` 校验接收配额并累计 `unacked_bytes`，`maybeEmitWindowUpdate` 达阈值自动生成 13 字节 `WINDOW_UPDATE` 帧并补窗。
- **RFC 9113 §7 完整错误码映射**：覆盖 `NoError(0)` 至 `Http11Required(13)` 全部标准错误码。

---

## 导出清单

### 常量与枚举

- `H2_MAX_WINDOW_SIZE = 2147483647`、`H2_DEFAULT_INITIAL_WINDOW = 65535`、`H2_RST_STREAM_FRAME_LEN = 13`、`H2_WINDOW_UPDATE_FRAME_LEN = 13`
- `StreamState`：`Open | HalfClosedLocal | HalfClosedRemote | Closed`
- `H2ErrorCode`：`NoError | ProtocolError | InternalError | FlowControlError | SettingsTimeout | StreamClosed | FrameSizeError | RefusedStream | Cancel | CompressionError | ConnectError | EnhanceYourCalm | InadequateSecurity | Http11Required`

### 核心结构体与函数

| 签名 | 说明 |
| :--- | :--- |
| `errorCodeToU32(code): u32` | 将 `H2ErrorCode` 转为 RFC 9113 线格式 `u32` |
| `encodeRstStreamFrame(stream_id, code, out_buf): usize?` | 编码 13 字节 `RST_STREAM` 帧 |
| `encodeWindowUpdateFrame(stream_id, increment, out_buf): usize?` | 编码 13 字节 `WINDOW_UPDATE` 帧 |
| `H2Stream.init(id, initial_send_window, initial_recv_window): H2Stream?` | 创建校验后的 `Open` HTTP/2 流 |
| `localClosed()` / `remoteClosed()` / `isClosed()` / `canSendData()` | 流状态与发送就绪查询 |
| `sentEndStream(*_)` / `recvEndStream(*_)` / `markHeadersReceived(*_, status, end_stream)` | RFC 9113 流状态推进 |
| `reset(*_, code, out_rst_frame): usize?` | 幂等重置流并生成 `RST_STREAM` 帧 |
| `consumeSendWindow(*_, bytes)` / `applySendWindowUpdate(*_, inc)` / `applySettingsInitialWindowDelta(*_, delta)` | 发送流控窗口管理 |
| `recordReceivedData(*_, bytes, end_stream)` / `maybeEmitWindowUpdate(*_, threshold, out_frame)` | 接收流控窗口与阈值自动补窗 |

---

## 使用示例

```typescript
import std from "std";
import {
    H2_DEFAULT_INITIAL_WINDOW,
    H2ErrorCode,
    H2Stream,
} from "cap:h2stream";

fn h2StreamDemo() {
    let st = H2Stream.init(1, H2_DEFAULT_INITIAL_WINDOW, H2_DEFAULT_INITIAL_WINDOW) ?? return;
    * = st.consumeSendWindow(4096);
    st.sentEndStream();
    * = st.markHeadersReceived(200, false);
    * = st.recordReceivedData(32768, false);

    let frame: [13]u8 = std.mem.zeroes([13]u8);
    let out: []u8 = frame[0..];
    const wu_len = st.maybeEmitWindowUpdate(32768, &out);
    * = wu_len;

    const rst_len = st.reset(H2ErrorCode.Cancel, &out);
    * = rst_len;
}
```

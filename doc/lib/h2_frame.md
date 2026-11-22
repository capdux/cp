# `cap:h2_frame` — HTTP/2 (RFC 9113) 线协议帧头、设置与优先级编解码器

> 参考来源：/data/dev/bun/src/http/H2FrameParser.cp | 吸收日期：2026-11-22

源自 Bun `http/H2FrameParser.cp`，纯 CP 零依赖实现 HTTP/2 客户端连接前言校验、9 字节大端帧头、5 字节流优先级与 6 字节 SETTINGS 单元编解码，以及完整帧类型/错误码/设置 ID 枚举。

## 导出符号

- 常量：`CLIENT_PREFACE`、`FRAME_HEADER_SIZE`(9)、`SETTINGS_UNIT_SIZE`(6)、`STREAM_PRIORITY_SIZE`(5)、`FLAG_ACK`/`FLAG_END_STREAM`/`FLAG_END_HEADERS`/`FLAG_PADDED`/`FLAG_PRIORITY`、`MAX_WINDOW_SIZE`/`MAX_STREAM_ID`/`DEFAULT_MAX_FRAME_SIZE`
- 枚举：`FrameType`、`ErrorCode`、`SettingsType`、`H2FrameError`
- 编解码结构：`UInt31WithReserved`、`FrameHeader`（`encode`/`decode`/`hasFlag`/`isEndStream`/`isEndHeaders`/`isAck`）、`StreamPriority`、`SettingsUnit`、`hasClientPreface`

## 上游行为契约与线协议安全边界

1. **大端 9 字节帧头与 R 保留位屏蔽（RFC 9113 §4.1）**：
   - `FrameHeader` 固定 9 字节网络序（24-bit 长度 + 8-bit 类型 + 8-bit 标志 + 1-bit 保留位 + 31-bit Stream ID）；`UInt31WithReserved` 发送时保留位清零、接收时按规范忽略最高保留位，保证 `stream_id <= MAX_STREAM_ID`（`0x7FFFFFFF`）。
2. **连接前言与流控窗口边界**：
   - 服务端握手前须用 `hasClientPreface` 严格校验 24 字节 `"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"` 前言；
   - 默认帧载荷上限 `DEFAULT_MAX_FRAME_SIZE`（16384 字节），流控窗口严禁超过 `MAX_WINDOW_SIZE`（`2^31 - 1`），违例须返回 `ErrorCode.FLOW_CONTROL_ERROR` 或 `FRAME_SIZE_ERROR`。
3. **SETTINGS 帧 6 字节对齐**：
   - 每个 `SettingsUnit` 固定 6 字节（16-bit ID + 32-bit 值），含 `FLAG_ACK` 标志的 SETTINGS 帧载荷长度必须为 0。

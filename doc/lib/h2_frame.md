# `cap:h2_frame` — HTTP/2 (RFC 9113) 线协议帧头、设置与优先级编解码器

源自 Bun `http/H2FrameParser.cp`，纯 CP 零依赖实现 HTTP/2 客户端连接前言校验、9 字节大端帧头、5 字节流优先级与 6 字节 SETTINGS 单元编解码，以及完整帧类型/错误码/设置 ID 枚举。

## 导出符号

- 常量：`CLIENT_PREFACE`、`FRAME_HEADER_SIZE`(9)、`SETTINGS_UNIT_SIZE`(6)、`STREAM_PRIORITY_SIZE`(5)、`FLAG_ACK`/`FLAG_END_STREAM`/`FLAG_END_HEADERS`/`FLAG_PADDED`/`FLAG_PRIORITY`、`MAX_WINDOW_SIZE`/`MAX_STREAM_ID`/`DEFAULT_MAX_FRAME_SIZE`
- 枚举：`FrameType`、`ErrorCode`、`SettingsType`、`H2FrameError`
- 编解码结构：`UInt31WithReserved`、`FrameHeader`（`encode`/`decode`/`hasFlag`/`isEndStream`/`isEndHeaders`/`isAck`）、`StreamPriority`、`SettingsUnit`、`hasClientPreface`

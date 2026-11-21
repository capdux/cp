# `cap:mysql_handshake` — MySQL 4.1 握手响应报文编解码器

源自 Bun `sql/mysql/protocol/HandshakeResponse41.cp`，纯 CP 实现 MySQL 4.1 `HandshakeResponse41` 完整帧编解码（能力标志协商、23 字节保留填充、安全连接/LenEnc 认证响应、数据库名、认证插件名与连接属性表），输出结构具备 `~init()` 确定性 RAII 零泄漏释放。

## 导出符号

- 能力标志常量：`CAP_LONG_PASSWORD`、`CAP_CONNECT_WITH_DB`、`CAP_PROTOCOL_41`、`CAP_SECURE_CONNECTION`、`CAP_PLUGIN_AUTH`、`CAP_CONNECT_ATTRS`、`CAP_PLUGIN_AUTH_LENENC`
- 类型：`HandshakeError`、`ConnectAttr`、`HandshakeResponse41`、`EncodedHandshake`（含 `~init()`）、`DecodedHandshake41`
- 编解码函数：`encodeHandshakeResponse41`、`decodeHandshakeResponse41`

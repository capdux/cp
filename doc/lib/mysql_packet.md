# `cap:mysql_packet` — MySQL 4 字节报文帧编解码与流式读写器

源自 Bun `sql/mysql/protocol/{NewWriter,NewReader}.cp`，提供 4 字节小端 `PacketHeader`、带 `~init()` 确定性 RAII 释放与自动回填 24 位负载长度头的 `MySqlPacketWriter`，以及零拷贝 `MySqlPacketReader`（小端整型、Length-Encoded Int/String、C 字符串）。

## 导出符号

- `PACKET_HEADER_SIZE`(4)、`MAX_PACKET_PAYLOAD`、`MySqlPacketError`、`PacketHeader`、`DecodedLenInt`、`decodeLenInt`
- `MySqlPacketReader`: `init` / `remaining` / `peek` / `skip` / `readBytes` / `readHeader` / `readU8` / `readU16Le` / `readU32Le` / `readU64Le` / `readLenInt` / `readLenString` / `readCString`
- `PacketHandle`、`MySqlPacketWriter`（含 `~init()` RAII）: `init` / `bytes` / `offset` / `reset` / `write` / `writeU8`..`writeU64Le` / `writeLenInt` / `writeLenString` / `writeCString` / `startPacket` / `endPacket`

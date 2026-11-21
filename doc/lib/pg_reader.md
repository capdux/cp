# `cap:pg_reader` — PostgreSQL v3 零拷贝大端消息帧读取器

源自 Bun `sql/postgres/protocol/NewReader.cp`，纯 CP 零堆分配实现 PostgreSQL v3 后端消息帧（`tag + i32_be_len`）解析、大端整型读写、NUL 终止 C 字符串与可空长度前缀字段（`-1 => null`）解码。

## 导出符号

- `FRAME_HEADER_SIZE`(5)、`PgReaderError`、`PgBackendTag`、`PgFrame`
- `PgMessageReader`: `init` / `markMessageStart` / `rewindToMessageStart` / `remaining` / `isEof` / `peek` / `ensureCapacity` / `skip` / `readBytes` / `readU8` / `readU16Be` / `readI16Be` / `readU32Be` / `readI32Be` / `readI64Be` / `peekI32Be` / `readCString` / `readNullableField` / `nextFrame`

# `cap:mysql_control` — MySQL 协议控制与握手报文解码器

> 溯源：从 `/data/dev/bun/src/sql/mysql/protocol/{HandshakeV10,OKPacket,ErrorPacket,EOFPacket,AuthSwitchRequest,CommandType}.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:mysql_control` 提供零外部依赖、零堆分配的 MySQL 客户端/服务端控制报文解码器，所有字符串与负载字段均零拷贝切片引用原始输入缓冲区。

---

## 快速导入

```cp
import {
    HandshakeV10,
    OKPacket,
    ErrorPacket,
    EOFPacket,
    AuthSwitchRequest,
    CommandType,
    Error,
} from "cap:mysql_control";
```

---

## API 参考

### 1. `Error` & `CommandType`

- `Error`：`TruncatedPacket`, `UnsupportedProtocolVersion`, `InvalidOKPacket`, `InvalidErrorPacket`, `InvalidEOFPacket`, `InvalidAuthSwitchRequest`, `InvalidLengthEncodedInt`
- `CommandType: u8`：完整 31 个 MySQL 客户端命令字节（`COM_QUIT = 0x01` .. `COM_RESET_CONNECTION = 0x1F`），提供 `fromByte(b): CommandType?`、`asByte(): u8`、`isPreparedStmt(): bool`

### 2. 控制与握手报文解码结构体

| 结构体 | 解码方法 | 核心字段与辅助方法 |
| :--- | :--- | :--- |
| `HandshakeV10` | `HandshakeV10.decode(buf): HandshakeV10!` | `protocol_version`, `server_version`, `connection_id`, `auth_plugin_data_part_1`, `auth_plugin_data_part_2`, `capability_flags` (32-bit 合并), `character_set`, `status_flags`, `auth_plugin_name`; `writeAuthPluginData(out)`, `supportsPluginAuth()`, `supportsSsl()` |
| `OKPacket` | `OKPacket.decode(buf): OKPacket!` | `header` (`0x00`/`0xFE`), `affected_rows`, `last_insert_id`, `status_flags`, `warnings`, `info`; `isEofStyle()` |
| `ErrorPacket` | `ErrorPacket.decode(buf): ErrorPacket!` | `header` (`0xFF`), `error_code`, `has_sql_state`, `sql_state`, `error_message`; `sqlStateSlice(): string?` |
| `EOFPacket` | `EOFPacket.decode(buf): EOFPacket!` | `header` (`0xFE`), `warnings`, `status_flags` |
| `AuthSwitchRequest` | `AuthSwitchRequest.decode(buf): AuthSwitchRequest!` | `header` (`0xFE`), `plugin_name`, `plugin_data` |

---

## 使用示例

```cp
import { HandshakeV10, OKPacket, ErrorPacket } from "cap:mysql_control";

fn handlePacket(payload: []const u8)! {
    const hs = HandshakeV10.decode(payload)?;
    let salt: [20]u8 = [0; 20];
    const salt_len = hs.writeAuthPluginData(salt[0..20]);
    * = salt_len;
}
```

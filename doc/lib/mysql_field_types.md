# `cap:mysql_field_types` — 纯 CP MySQL 协议列类型与 ColumnDefinition41 元数据编解码库

## 概述

`cap:mysql_field_types` 吸收自 Bun `src/sql/mysql/MySQLTypes.cp` 与 `src/sql/mysql/protocol/ColumnDefinition41.cp`，纯 CP 零外部依赖，完整提供 MySQL 31 种协议列类型 `FieldType`、16 位列标志位域 `ColumnFlags`、MySQL length-encoded integer/string 编解码以及完整 `ColumnDefinition41` 列元数据报文编解码（RAII `MySqlWireBuffer` 自动释放内存）。

## 核心类型与 API

### 字符集常量与错误集

- `CHARSET_LATIN1_SWEDISH_CI` (8) / `CHARSET_UTF8MB3_GENERAL_CI` (33) / `CHARSET_UTF8MB4_GENERAL_CI` (45) / `CHARSET_UTF8MB4_BIN` (46) / `CHARSET_BINARY` (63) / `CHARSET_UTF8MB4_UNICODE_CI` (224)
- `MySqlWireError`：`UnexpectedEof` / `InvalidFieldType` / `InvalidLengthEncodedInt` / `InvalidFixedFieldsLength` / `BufferTooSmall` / `OutOfMemory`

### `FieldType`（全部 31 变体）

- 枚举值：`MYSQL_TYPE_DECIMAL (0x00)` .. `MYSQL_TYPE_TIME2 (0x13)` 与 `MYSQL_TYPE_JSON (0xf5)` .. `MYSQL_TYPE_GEOMETRY (0xff)`
- 方法：`toByte()`、`isBinaryFormatSupported()`、`isInteger()`、`isFloat()`、`isString()`、`isTemporal()`、`name()`
- 解析：`fieldTypeFromByte(b: u8): FieldType ! MySqlWireError`

### `ColumnFlags`（16 位位域）

- 字段：`not_null` / `pri_key` / `unique_key` / `multiple_key` / `blob` / `unsigned` / `zerofill` / `binary` / `is_enum` / `auto_increment` / `timestamp` / `is_set` / `no_default_value` / `on_update_now` / `num` / `part_key`
- 方法：`toInt(): u16`、`ColumnFlags.fromInt(raw: u16): ColumnFlags`

### Length-Encoded 编解码与 `ColumnDefinition41`

- `lenEncIntSize` / `encodeLenEncInt` / `decodeLenEncInt`
- `encodeLenEncString` / `decodeLenEncString`
- `MySqlWireBuffer`（RAII `~init()` 自动释放）
- `ColumnDefinition41` + `encodeColumnDefinition41(alloc, col)` / `decodeColumnDefinition41(data)`

## 示例

```cp
import {
    FieldType,
    ColumnFlags,
    ColumnDefinition41,
    encodeColumnDefinition41,
    decodeColumnDefinition41,
} from "cap:mysql_field_types";

let wire = encodeColumnDefinition41(alloc, ColumnDefinition41{
    schema: "shop",
    table: "users",
    org_table: "users",
    name: "id",
    org_name: "id",
    column_length: 20,
    column_type: FieldType.MYSQL_TYPE_LONGLONG,
    flags: ColumnFlags{ not_null: true, pri_key: true, unsigned: true, auto_increment: true },
})?;
const decoded = decodeColumnDefinition41(wire.slice())?;
// decoded.column.column_type.isInteger() == true
```

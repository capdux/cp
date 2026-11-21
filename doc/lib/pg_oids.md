# `cap:pg_oids` — PostgreSQL OID 类型目录与分类工具

> 来源：吸收自 `/data/dev/bun/src/sql/postgres/types/Tag.cp` 与 `PostgresTypes.cp`

`cap:pg_oids` 提供纯 CP、零外部依赖、零堆分配的 PostgreSQL 内置标量与数组 OID 类型目录（45 项高频 OID）、类型大类分类、标量↔数组 OID 双向映射与二进制线协议编码偏好查询。

## 核心类型

### `PgTypeCategory`

```cp
pub enum PgTypeCategory: u8 {
    boolean = 0,
    integer = 1,
    floating = 2,
    numeric = 3,
    text = 4,
    binary = 5,
    json = 6,
    datetime = 7,
    network = 8,
    uuid = 9,
    array = 10,
    other = 11,

    fn asStr(): string
}
```

### `PgOid`

```cp
pub enum PgOid: u32 {
    BOOL = 16, BYTEA = 17, CHAR = 18, NAME = 19, INT8 = 20, INT2 = 21, INT4 = 23,
    TEXT = 25, OID = 26, JSON = 114, XML = 142, CIDR = 650, FLOAT4 = 700, FLOAT8 = 701,
    MONEY = 790, MACADDR = 829, INET = 869, BPCHAR = 1042, VARCHAR = 1043, DATE = 1082,
    TIME = 1083, TIMESTAMP = 1114, TIMESTAMPTZ = 1184, INTERVAL = 1186, TIMETZ = 1266,
    BIT = 1560, VARBIT = 1562, NUMERIC = 1700, UUID = 2950, JSONB = 3802,
    // 对应数组 OID:
    JSON_ARRAY = 199, BOOL_ARRAY = 1000, BYTEA_ARRAY = 1001, INT2_ARRAY = 1005,
    INT4_ARRAY = 1007, TEXT_ARRAY = 1009, VARCHAR_ARRAY = 1015, INT8_ARRAY = 1016,
    FLOAT4_ARRAY = 1021, FLOAT8_ARRAY = 1022, TIMESTAMP_ARRAY = 1115,
    TIMESTAMPTZ_ARRAY = 1185, NUMERIC_ARRAY = 1231, UUID_ARRAY = 2951, JSONB_ARRAY = 3807,

    static fn fromU32(oid_val: u32): PgOid?
    fn toU32(): u32
    fn typeName(): []const u8
    fn category(): PgTypeCategory
    fn isArray(): bool
    fn elementOid(): PgOid?
    fn arrayOid(): PgOid?
    fn prefersBinary(): bool
}
```

## 使用示例

```cp
import { PgOid, PgTypeCategory } from "cap:pg_oids";

const oid = PgOid.fromU32(1007).?;         // PgOid.INT4_ARRAY
const elem = oid.elementOid().?;           // PgOid.INT4
const back = elem.arrayOid().?;            // PgOid.INT4_ARRAY
// elem.category() == .integer, elem.prefersBinary() == true
```

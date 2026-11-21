# `cap:mysql_proto` — MySQL 协议基础类型与常量

> 溯源：从 `/data/dev/bun/src/sql/mysql/Capabilities.cp`、`StatusFlags.cp` 与 `protocol/CharacterSet.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:mysql_proto` 提供 MySQL 客户端/服务端通信协议所需的核心零分配协议结构体与枚举：

- **`Capabilities` 与 32 位 `CAP_*` 掩码常量**：完整建模 MySQL 握手阶段的 32 位能力标志位，支持位掩码编解码、能力交/并集协商、默认客户端配置及不支持特性剔除。
- **`StatusFlag` 与 `StatusFlags`**：建模 MySQL 服务端 OK/EOF 报文中的 16 位状态标志位，提供事务、只读事务、多结果集、自动提交、游标、慢查询等快捷查询与修改接口。
- **`CharacterSet`**：完整覆盖 MySQL 标准 Collation ID（1..250 及 MySQL 8.0 常用 `utf8mb4_0900_*`），提供安全的 `fromId` 查表、字符集归属、单字符最大字节数（`MAXLEN`）及 UTF-8 / Binary 判定。

---

## 快速导入

```cp
import {
    Capabilities,
    CAP_CLIENT_PROTOCOL_41,
    CAP_CLIENT_SSL,
    CAP_CLIENT_CONNECT_WITH_DB,
    CAP_CLIENT_MULTI_RESULTS,
    StatusFlag,
    StatusFlags,
    CharacterSet,
} from "cap:mysql_proto";
```

---

## API 参考

### 1. `Capabilities`

| 成员 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| 32 个布尔字段 | `CLIENT_LONG_PASSWORD: bool = false` .. `CLIENT_REMEMBER_OPTIONS: bool = false` | 对应 MySQL 32 位能力位（bit 0 ~ bit 31） |
| `CAP_*` 常量 | `pub const CAP_CLIENT_*: u32` | 32 个对应的位掩码常量 |
| `toU32` | `fn toU32(): u32` | 编码为 32 位无符号整数位掩码 |
| `fromU32` | `static fn fromU32(flags: u32): Capabilities` | 从 32 位位掩码解码 |
| `defaultClient` | `static fn defaultClient(ssl: bool, has_db_name: bool): Capabilities` | 生成推荐默认客户端能力集（启用 Protocol 4.1、Plugin Auth、Secure Connection、Deprecate EOF、Multi Statements/Results） |
| `hasFlag` | `fn hasFlag(mask: u32): bool` | 检测是否包含指定单个或组合位掩码（`mask == 0` 返回 `false`） |
| `intersect` | `fn intersect(other: Capabilities): Capabilities` | 计算两个能力集的按位交集（用于握手协商） |
| `unionWith` | `fn unionWith(other: Capabilities): Capabilities` | 计算两个能力集的按位并集 |
| `rejectUnsupported` | `fn rejectUnsupported(*_)` | 清除压缩、MFA、扩展等不支持的位 |

### 2. `StatusFlag` & `StatusFlags`

| 成员 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `StatusFlag` | `pub enum StatusFlag: u16` | MySQL 14 个标准服务端状态标志枚举 |
| `toU16` | `fn toU16(): u16` | 返回底层 16 位状态掩码 |
| `fromU16` | `static fn fromU16(flags: u16): StatusFlags` | 从 16 位状态掩码构造 |
| `has` | `fn has(flag: StatusFlag): bool` | 检查指定标志是否置位 |
| `set` / `clear` | `fn set(*_, flag: StatusFlag)` / `fn clear(*_, flag: StatusFlag)` | 置位或清除指定标志 |
| `inTransaction` | `fn inTransaction(): bool` | 是否处于事务中（含只读事务） |
| `inReadOnlyTransaction` | `fn inReadOnlyTransaction(): bool` | 是否处于只读事务中 |
| `isAutoCommit` | `fn isAutoCommit(): bool` | 是否开启自动提交 |
| `hasMoreResults` | `fn hasMoreResults(): bool` | 是否存在更多待读取结果集 |
| `cursorExists` | `fn cursorExists(): bool` | 是否存在服务端游标 |
| `lastRowSent` | `fn lastRowSent(): bool` | 是否已发送最后一行 |
| `isSlowQuery` | `fn isSlowQuery(): bool` | 是否标记为慢查询 |
| `isSessionStateChanged` | `fn isSessionStateChanged(): bool` | 会话状态是否发生变更 |

### 3. `CharacterSet`

| 成员 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `default` | `const default = CharacterSet.utf8mb4_general_ci` | 默认推荐字符集（ID 45） |
| `id` | `fn id(): u16` | 返回底层 MySQL Collation ID |
| `name` | `fn name(): string` | 返回完整 collation 名称（如 `"utf8mb4_general_ci"`） |
| `charset` | `fn charset(): string` | 返回基础字符集名称（如 `"utf8mb4"` / `"latin1"` / `"binary"`） |
| `maxBytesPerChar` | `fn maxBytesPerChar(): u8` | 返回该字符集单字符最大字节数（1 / 2 / 3 / 4） |
| `isUtf8` | `fn isUtf8(): bool` | 是否属于 UTF-8 家族（`utf8mb4` 或 `utf8mb3`） |
| `isUtf8mb4` | `fn isUtf8mb4(): bool` | 是否为完整 4 字节 `utf8mb4` |
| `isBinary` | `fn isBinary(): bool` | 是否为 `binary`（ID 63） |
| `fromId` | `static fn fromId(id: u16): CharacterSet?` | 从 Collation ID 安全查找，未知或空洞 ID 返回 `null` |

---

## 使用示例

```cp
import {
    Capabilities,
    CAP_CLIENT_SSL,
    CAP_CLIENT_PROTOCOL_41,
    StatusFlag,
    StatusFlags,
    CharacterSet,
} from "cap:mysql_proto";

fn negotiate(server_caps_raw: u32, ok_status_raw: u16, collation_id: u16) {
    let client_caps = Capabilities.defaultClient(true, true);
    let server_caps = Capabilities.fromU32(server_caps_raw);
    let negotiated = client_caps.intersect(server_caps);
    negotiated.rejectUnsupported();

    const use_ssl = negotiated.hasFlag(CAP_CLIENT_SSL | CAP_CLIENT_PROTOCOL_41);
    * = use_ssl;

    const status = StatusFlags.fromU16(ok_status_raw);
    if (status.inTransaction() && status.hasMoreResults()) {
        // 事务中存在后续结果集
    }

    if (cs := CharacterSet.fromId(collation_id)) {
        const charset_name = cs.charset();
        const max_len = cs.maxBytesPerChar();
        * = charset_name;
        * = max_len;
    }
}
```

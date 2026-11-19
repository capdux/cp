# CP 标准库：Redis / Valkey 协议引擎与流水线命令构建手册 (`cap:redis`)

## 一、 架构总览与设计准则

`cap:redis`（以及底层同源协议层 `lib/valkey/`）是 CP 语言官方内置的工业级 Redis / Valkey 序列化协议（RESP2 / RESP3）标准库，全面吸收自 Bun 原生 Valkey 协议实现（`/data/dev/bun/src/valkey/valkey_protocol.cp`）。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     批处理流水线 (RedisPipeline)   │     高频命令构建器 (RedisCommand)  │
│  4KB 栈内联缓冲批量打包多条指令    │  PING / AUTH / GET / SET / DEL     │
│  ping / get / set / hset / lpush  │  EXPIRE / HSET / HGET / LPUSH /RPOP│
├───────────────────────────────────┼────────────────────────────────────┤
│     RESP 报文编码器 (RESPEncoder)  │  零拷贝流式解析器 (ValkeyReader)   │
│  BulkString / Array / Map / Set   │  RESPParser.parseFrame / nextFrame │
├───────────────────────────────────┴────────────────────────────────────┤
│            RESP2 & RESP3 全类型系统 (RESPType / RESPFrame)             │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/redis/` 与 `lib/valkey/` 下所有源文件第 1 行均严格标记上游 Bun 源码来源：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/redis/resp.cp` | `// Source: /data/dev/bun/src/valkey/valkey_protocol.cp` | `RESPType` (RESP2/RESP3 15 种帧类型)、`RESPFrame`、`RESPParser`、`ValkeyReader` 流式零拷贝读取器、`RESPEncoder` 编码器 |
| `lib/redis/command.cp` | `// Source: /data/dev/bun/src/valkey/valkey_protocol.cp` | `RedisCommand` 常用 Redis 指令零分配编码器与 `RedisPipeline` 4KB 栈内联批处理流水线 |
| `lib/redis/index.cp` | `// Source: /data/dev/bun/src/valkey/valkey_protocol.cp` | `cap:redis` 标准库对外统一导出入口 |
| `lib/valkey/valkey_protocol_.cp` | `// Source: /data/dev/bun/src/valkey/valkey_protocol.cp` | `VerbatimString` (`txt:`/`mkd:` 零拷贝视图) 与 `SubscriptionPushMessage` (Pub/Sub 推送帧枚举) |
| `lib/valkey/valkey_protocol.cp` | `// Source: /data/dev/bun/src/valkey/valkey_protocol.cp` | Valkey 与 Redis 协议层统一聚合入口 |

---

## 二、 RESP2 / RESP3 协议报文格式矩阵

`RESPType` 完整覆盖 RESP2 与 RESP3 规范定义的全部 15 种首字节类型标签：

| 协议版本 | 枚举值 (`RESPType`) | 首字节前缀 | 报文格式示例 | `RESPFrame` 提取字段 |
| :---: | :--- | :---: | :--- | :--- |
| **RESP2** | `SimpleString` | `'+'` | `+OK\r\n` | `frame.str_val` (`"OK"`) |
| **RESP2** | `Error` | `'-'` | `-ERR unknown command\r\n` | `frame.str_val` |
| **RESP2** | `Integer` | `':'` | `:1024\r\n` / `:-42\r\n` | `frame.int_val` (`1024`, `-42`) |
| **RESP2** | `BulkString` | `'$'` | `$5\r\nhello\r\n` / `$-1\r\n` | `frame.str_val` / `frame.is_null` |
| **RESP2** | `Array` | `'*'` | `*2\r\n$3\r\nGET\r\n$1\r\nk\r\n` | `frame.array_count` (`2`) |
| **RESP3** | `Null` | `'_'` | `_\r\n` | `frame.is_null` (`true`) |
| **RESP3** | `Boolean` | `'#'` | `#t\r\n` / `#f\r\n` | `frame.bool_val` (`true` / `false`) |
| **RESP3** | `Double` | `','` | `,3.14159\r\n` | `frame.str_val` (`"3.14159"`) |
| **RESP3** | `BigNumber` | `'('` | `(3492890328409238509324850943850943825024385\r\n` | `frame.str_val` |
| **RESP3** | `BlobError` | `'!'` | `!21\r\nSYNTAX invalid syntax\r\n` | `frame.str_val` |
| **RESP3** | `VerbatimString` | `'='` | `=15\r\ntxt:Some string\r\n` | `frame.str_val` -> `VerbatimString.parse` |
| **RESP3** | `Map` | `'%'` | `%2\r\n+first\r\n:1\r\n+second\r\n:2\r\n` | `frame.array_count` (`2` 个键值对) |
| **RESP3** | `Set` | `'~'` | `~3\r\n+orange\r\n+apple\r\n+one\r\n` | `frame.array_count` (`3`) |
| **RESP3** | `Attribute` | `'|'` | `|1\r\n+key-popularity\r\n...` | `frame.array_count` |
| **RESP3** | `Push` | `'>'` | `>3\r\n+message\r\n+chan\r\n+msg\r\n` | `frame.array_count` (`3`) |

---

## 三、 命令构建器、流水线与流式响应解析器详解

### 1. 单指令编码与 `RedisPipeline` 批量流水线

在网络高吞吐场景下，通过 `RedisPipeline` 将多条 Redis 命令打包进单次 TCP `write` 缓冲区，可消除多次 RTT 往返延迟：

```typescript
import {
    RedisCommand,
    RedisPipeline,
    RESPEncoder,
} from "cap:redis";

pub fn demoRedisCommandsAndPipeline() {
    // 1. 单条指令零堆分配编码
    let cmd_buf: [128]u8 = undefined;
    const n_set = RedisCommand.encodeSet(&cmd_buf, "session:101", "active");
    // cmd_buf[0..n_set] => "*3\r\n$3\r\nSET\r\n$11\r\nsession:101\r\n$6\r\nactive\r\n"

    // 2. 4KB 栈内联流水线批处理 (Pipeline)
    let pipe = RedisPipeline.init();
    * = pipe.auth("s3cr3t_pass");
    * = pipe.set("user:1:name", "Alice");
    * = pipe.expire("user:1:name", 3600);
    * = pipe.hset("user:1:profile", "role", "architect");
    * = pipe.hget("user:1:profile", "role");
    * = pipe.lpush("jobs:queue", "task_99");
    * = pipe.rpop("jobs:queue");
    * = pipe.ping();

    // pipe.command_count == 8
    // pipe.bytes() 返回拼接好的完整 RESP 字节流切片，可直接写入 Socket
}
```

### 2. 零拷贝流式响应解析器 (`RESPParser` & `ValkeyReader`)

`ValkeyReader` 在 TCP 接收缓冲区上维护游标 `pos`，通过 `nextFrame()` 逐帧零拷贝解析服务端返回的流水线响应或复合结构（Array / Map / Push）：

```typescript
import {
    RESPType,
    RESPParser,
    ValkeyReader,
    VerbatimString,
} from "cap:redis";

pub fn demoStreamReader() {
    // 模拟从 Socket 一次性收到的多条 Pipeline 响应帧 (含 RESP2 与 RESP3)
    const raw_stream =
        "+OK\r\n" +
        ":42\r\n" +
        "$11\r\nAntigravity\r\n" +
        "#t\r\n" +
        "_\r\n" +
        "=14\r\ntxt:hello resp\r\n";

    let reader = ValkeyReader.init(raw_stream);

    // Frame 1: +OK
    if (f1 := reader.nextFrame()) {
        // f1.kind == RESPType.SimpleString, f1.str_val == "OK"
    }

    // Frame 2: :42
    if (f2 := reader.nextFrame()) {
        // f2.kind == RESPType.Integer, f2.int_val == 42
    }

    // Frame 3: $11\r\nAntigravity\r\n
    if (f3 := reader.nextFrame()) {
        // f3.kind == RESPType.BulkString, f3.str_val == "Antigravity"
    }

    // Frame 4: #t (RESP3 Boolean)
    if (f4 := reader.nextFrame()) {
        // f4.kind == RESPType.Boolean, f4.bool_val == true
    }

    // Frame 5: _ (RESP3 Null)
    if (f5 := reader.nextFrame()) {
        // f5.kind == RESPType.Null, f5.is_null == true
    }

    // Frame 6: =14\r\ntxt:hello resp\r\n (RESP3 VerbatimString)
    if (f6 := reader.nextFrame()) {
        if (vb := VerbatimString.parse(f6.str_val)) {
            // vb.format == "txt", vb.content == "hello resp"
        }
    }
}
```

---

## 四、 与 `Bun.redis` / `Bun.valkey` API 对照表

| 功能特性 | Bun (`Bun.redis` / `src/valkey/valkey_protocol.cp`) | CP 标准库 (`cap:redis`) |
| :--- | :--- | :--- |
| **RESP 类型识别** | `RESPType.fromByte(byte)` | `RESPType.fromByte(byte)` |
| **单帧零拷贝解析** | `ValkeyReader.readValue()` | `RESPParser.parseFrame(buf)` |
| **流式多帧游标读取** | `ValkeyReader.init(buf)` | `ValkeyReader.init(buf)` -> `reader.nextFrame()` |
| **RESP3 Verbatim 解析** | `reader.readVerbatimString()` | `VerbatimString.parse(frame.str_val)` |
| **Pub/Sub 推送分类** | `SubscriptionPushMessage.map` | `SubscriptionPushMessage.fromString(kind)` |
| **基础命令编码** | `ValkeyClient` 内部格式化 | `RedisCommand.encodeGet` / `encodeSet` / `encodeHSet` / `encodeExpire` 等 |
| **流水线批量打包** | 自动 Auto-Pipelining 队列 | `RedisPipeline.init()` -> `pipe.set(...)` / `pipe.get(...)` -> `pipe.bytes()` |

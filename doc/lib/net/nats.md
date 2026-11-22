# CP 标准库：NATS 线协议编码与增量流式解析器 (`cap:nats`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:net`，导入请使用 `import { ... } from "cap:net"`。

## 一、 架构总览与设计准则

`cap:nats` 是 CP 语言官方内置的 **NATS（Neural Autonomic Transport System）** 纯 CP 线协议编码与增量流式拆帧标准库。严格遵循 NATS 文本控制行 + 二进制载荷定界与 `NATS/1.0` 消息头规范，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:net"` 静态导入，也支持 `import cap from "cap"; cap.nats.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│             CP NATS 线协议编码与增量解析引擎 (cap:nats)                 │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ NatsEncoder           │ NATS/1.0 消息头         │ NatsParser 增量解析   │
│ CONNECT / SUB / UNSUB │ NatsHeader / NatsMsg    │ INFO / MSG / HMSG     │
│ PUB / HPUB            │ 多值头保留与大小写不敏感│ PING / PONG / +OK/-ERR│
│ PING / PONG           │ hdr_len <= total_len    │ 跨任意分块半包/黏包   │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/nats/nats.cp` | `// Source: cp/lib/nats/nats.cp (...)` | `NatsError`、`NatsHeader`、`NatsMsg`、`NatsOp`、`NatsEncoder`、`NatsParser` 完整实现 |
| `lib/nats/index.cp` | `// Source: cp/lib/nats/index.cp (...)` | `cap:nats` 与 `cap.nats` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集 `NatsError`

```typescript
pub err NatsError {
    InvalidProtocolLine, // 控制行格式错误、缺少 CRLF 或参数数量不合法
    InvalidSubject,      // 主题 / sid / reply_to 为空或包含空白字符
    InvalidPayloadSize,  // 载荷或头长度非合法非负十进制整数
    PayloadTooLarge,     // 载荷长度超出 max_payload_bytes 上限
    InvalidNatsHeaders,  // NATS/1.0 头格式错误或 hdr_bytes > total_bytes
    CommandInjection,    // 参数中包含 CR (\r) 或 LF (\n) 控制行注入
    OutOfMemory,         // 内存分配失败
}
```

### 2. 消息与操作类型 (`NatsHeader`, `NatsMsg`, `NatsOp`)

- `NatsHeader { name: []const u8, value: []const u8 }`：提供 `NatsHeader.init(name, value)`。
- `NatsMsg { subject: []const u8, sid: []const u8, reply_to: ([]const u8)?, headers: []const NatsHeader, payload: []const u8 }`：
  - `msg.deinit(alloc)`：释放消息内部所有堆分配切片。
  - `msg.header(name): ([]const u8)?`：按大小写不敏感方式检索首个匹配头部值。
- `NatsOp` 标签联合：
  - `info: []const u8`
  - `msg: NatsMsg`
  - `ping`
  - `pong`
  - `ok`
  - `err: []const u8`
  - 提供 `op.deinit(alloc)` 统一释放所有变体持有的内存。

### 3. 客户端命令编码器 (`NatsEncoder`)

| 方法签名 | 说明 |
| :--- | :--- |
| `NatsEncoder.encodeConnect(alloc, json_opts): ([]u8)!` | 编码 `CONNECT <json>\r\n`（拦截 CR/LF 注入） |
| `NatsEncoder.encodePub(alloc, subject, reply_to, payload): ([]u8)!` | 编码 `PUB <subject> [<reply_to> ]<#bytes>\r\n<payload>\r\n` |
| `NatsEncoder.encodeHpub(alloc, subject, reply_to, headers, payload): ([]u8)!` | 编码 `HPUB <subject> [<reply_to> ]<#hdr> <#total>\r\nNATS/1.0\r\n...\r\n\r\n<payload>\r\n` |
| `NatsEncoder.encodeSub(alloc, subject, queue_group, sid): ([]u8)!` | 编码 `SUB <subject> [<queue_group> ]<sid>\r\n` |
| `NatsEncoder.encodeUnsub(alloc, sid, max_msgs): ([]u8)!` | 编码 `UNSUB <sid>[ <max_msgs>]\r\n` |
| `NatsEncoder.encodePing(alloc): ([]u8)!` | 编码 `PING\r\n` |
| `NatsEncoder.encodePong(alloc): ([]u8)!` | 编码 `PONG\r\n` |

### 4. 增量流式解析器 (`NatsParser`)

| 方法签名 | 说明 |
| :--- | :--- |
| `NatsParser.init(alloc, max_payload_bytes): NatsParser` | 初始化解析器并设置单消息最大载荷上限 |
| `parser.deinit()` | 释放内部累积缓冲区 |
| `parser.feed(chunk): void!` | 喂入任意分块字节流 |
| `parser.nextOp(): (NatsOp?)!` | 解析下一条 `INFO`/`MSG`/`HMSG`/`PING`/`PONG`/`+OK`/`-ERR`，半包返回 `null` |
| `parser.bufferedLen(): usize` | 返回当前尚未消费的缓冲字节数 |

---

## 三、 使用示例

```typescript
const std = @import("std");
import { NatsEncoder, NatsParser, NatsHeader } from "cap:net";

pub fn example(alloc: std.mem.Allocator): void! {
    let hdrs: [1]NatsHeader = [NatsHeader.init("X-Trace", "abc-123")];
    const wire = NatsEncoder.encodeHpub(alloc, "orders.created", "reply.1", &hdrs, "{\"id\":1}")?;
    defer alloc.free(wire);

    let parser = NatsParser.init(alloc, 1024 * 1024);
    defer parser.deinit();

    parser.feed("MSG events.up 1 2\r\nhi\r\n")?;
    if (op := parser.nextOp()?) {
        defer op.deinit(alloc);
        // handle op
    }
}
```

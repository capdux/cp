# CP 标准库：核心运行时、并发 Actor、反射、序列化、测试与 SQLite 手册

> [!NOTE]
> 本子模块已归并至统一主包 `cap:napi`，导入请使用 `import { ... } from "cap:napi"`。

本文档统一涵盖 CP 语言 8 个顶层核心运行时与基础设施标准库：`cap:actor`、`cap:beam`、`cap:parallel`、`cap:worker`、`cap:reflect`、`cap:serde`、`cap:test` 与 `cap:sqlite`。

---

## 一、 架构总览与模块矩阵

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                     CP 核心运行时与基础能力全景图                         │
├───────────────────────────┬─────────────────────────────────────────────┤
│ 并发调度与消息传递底座     │  元编程、数据编解码、持久化与工程质量        │
│ • cap:beam   (BEAM 桥接)  │  • cap:reflect (零成本反射元数据)            │
│ • cap:actor  (Actor 协程) │  • cap:serde   (统一序列化门面 -> cap:json)  │
│ • cap:parallel (多核并行) │  • cap:sqlite  (SQLite3 C-ABI + RAII 自动关) │
│ • cap:worker (Worker 句柄)│  • cap:test    (原生零开销单元测试引擎)      │
└───────────────────────────┴─────────────────────────────────────────────┘
```

| 标准库协议 | 源码路径 | 来源谱系 | 核心职责 |
| :--- | :--- | :--- | :--- |
| `cap:beam` | `lib/beam.cp` | `Custom (CP native Bun-BEAM bridge)` | `SystemAction` 调度操作码、`PID`、`Message` 与 `Agent` BEAM NIF 桥接 |
| `cap:actor` | `lib/actor.cp` | `Custom (CP native actor runtime)` | `yield`、`receive`、`sleep`（双模 BEAM/Linux `nanosleep`）、`join`、`self_pid`、`sender`、`send`、`reply` |
| `cap:parallel` | `lib/parallel.cp` | `Custom (CP native async/parallel runtime)` | `Task` 句柄、`spawn(task_fn)` 线程池派发、`await join(t1, ...)` 多核并行汇聚 |
| `cap:worker` | `lib/worker.cp` | `Custom (CP native worker runtime)` | Worker 并发单元管理与 `PID` 类型桥接 |
| `cap:reflect` | `lib/reflect.cp` | `Custom (CP native stdlib)` | 编译期反射元数据结构：`FieldInfo`、`MethodInfo`、`TypeInfo` |
| `cap:serde` | `lib/serde.cp` | `/data/dev/bun/src/interchange/interchange.cp` | 统一序列化/反序列化门面，重导出 `cap:json` 全套 SIMD 零拷贝解析器与 `JSON` API |
| `cap:test` | `lib/test.cp` | `Custom (cap:test harness)` | 原生测试运行器入口 `test(name, cb)` 与 `expect_eq(actual, expected)` 断言宏 |
| `cap:sqlite` | `lib/sqlite.cp` | `Custom (cap:sqlite binding)` | 轻量级 SQLite3 C-ABI 封装 `Database`（`init` / `exec` / `query_string` + RAII 自动 `release()` 关闭连接） |

---

## 二、 分模块核心 API 详解

### 1. BEAM 调度桥接与 Actor 并发（`cap:beam`、`cap:actor`、`cap:worker`）

- **`cap:beam`**：
  - `SystemAction`：定义跨界委托给 BEAM 网关的系统操作码：`WAKE (1)`、`SLEEP (2)`、`STOP (3)`、`YIELD (4)`、网络组 `NET_CONNECT (10)..NET_LISTEN (13)`、文件组 `FS_READ (20)..FS_COPY (26)`、HTTP 组 `HTTP_FETCH (30)..HTTP_SERVE (31)` 与 `ERROR (99)`。
  - `PID { node_id: u32, local_id: u32 }`：全局分布式进程标识符。
  - `Message { sender: PID, action: u32, payload: *! }` 与 `Agent`（含 `send` / `onMessage`）。
- **`cap:actor`**：
  - `await yield()`：自愿推进 CPS 状态机 `pc` 并让出调度片；
  - `await sleep(ms)`：BEAM 模式发送 `SystemAction.SLEEP`，Native 模式直调 `std.os.linux.nanosleep`；
  - `await receive()` / `send(target, msg)` / `reply(target, msg)` / `self_pid()` / `sender()` / `await join(pid)`：一等公民 Actor 信箱原语。
- **`cap:worker`**：引入 `PID` 作为独立 Worker 线程/隔离单元的基础通信标识。

### 2. 单机多核并行编排（`cap:parallel`）

- `Task`：封装异步任务指针句柄；
- `spawn(task_fn)`：派发任务至多核 Worker 线程池，立即返回 `Task` 句柄；
- `await join(t1, t2, t3, t4)`：挂起当前协程直至所有任务完成并返回元组结果（如 `let [r1, r2] = await join(t1, t2);`）。

### 3. 零成本反射元数据（`cap:reflect`）

提供编译器反射系统产出的标准结构化视图：
- **`FieldInfo`**：`name: string`、`typeName: string`、`meta: *!`
- **`MethodInfo`**：`name: string`、`returnTypeName: string`、`paramNames: Array<string>`、`isAsync: bool`、`isStatic: bool`、`meta: *!`
- **`TypeInfo`**：`name: string`、`isRef: bool`、`fields: Array<FieldInfo>`、`methods: Array<MethodInfo>`、`meta: *!`

### 4. 统一序列化门面（`cap:serde`）

`cap:serde` 直接重导出 `lib/json/index.cp`，提供与 `cap:json` 完全一致的工业级编解码能力：
- **`JSON.parse(allocator, src): !JsonValue`**：零拷贝动态 DOM 解析（`JsonTag` / `JsonField` / `JsonValue`）；
- **`JSON.parseTyped(T, allocator, src): !T`**：编译期强类型反序列化，直接映射为 CP `struct`；
- **`JsonParser` / `FastJsonScanner` / `JsonBufferPool`**：底层高速扫描器与对象池。

### 5. 原生测试框架（`cap:test`）

- **`test(name: string, callback)`**：注册并运行独立测试沙箱（每次测试自动配备独立 `ArenaAllocator` 与异常捕获统计）；
- **`expect_eq(actual, expected)`**：强类型深度相等断言，失败时打印期望/实际值并计入 `cap test` 汇总报告。

### 6. 嵌入式关系数据库（`cap:sqlite`）

- **`Database`**：直连原生 `libsqlite3` C-ABI（`sqlite3_open`、`sqlite3_exec`、`sqlite3_prepare_v2`、`sqlite3_step`、`sqlite3_column_text`、`sqlite3_finalize`、`sqlite3_close`）；
- 核心方法：
  - `init(*_, path: string): void!`：打开磁盘或内存（`":memory:"`）数据库；
  - `exec(sql: string): void!`：执行 DDL / DML 语句；
  - `query_string(sql: string): string?!`：执行查询并返回首行首列文本；
  - `release(*_)`：作用域退出时由 CP 编译器 RAII 自动调用 `sqlite3_close`，杜绝句柄泄漏。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { test, expect_eq } from "cap:test";
import { SystemAction, PID } from "cap:beam";
import { self_pid } from "cap:actor";
import { JSON, JsonTag } from "cap:serde";

struct ServiceMeta {
    name: []const u8,
    port: i64,
    active: bool,
}

pub fn demoCoreRuntime(): !void {
    // 1. cap:serde 动态 DOM 与编译期强类型反序列化
    const raw = "{\"name\":\"cp-gateway\",\"port\":8080,\"active\":true}";
    const dom = try JSON.parse(std.heap.page_allocator, raw);
    expect_eq(dom.tag, JsonTag.Object);

    const cfg = try JSON.parseTyped(ServiceMeta, std.heap.page_allocator, raw);
    expect_eq(std.mem.eql(u8, cfg.name, "cp-gateway"), true);
    expect_eq(cfg.port, 8080);
    expect_eq(cfg.active, true);

    // 2. cap:beam 调度协议与 PID 值类型
    expect_eq(@intFromEnum(SystemAction.FS_READ), 20);
    expect_eq(@intFromEnum(SystemAction.HTTP_FETCH), 30);
    const pid = PID { node_id: 1, local_id: 1024 };
    expect_eq(pid.local_id, 1024);

    // 3. cap:actor 当前执行上下文 PID 查询
    const cur = self_pid();
    expect_eq(cur.local_id >= 0, true);
}
```

---

## 四、 与 Bun / Node.js / Erlang API 对照表

| 功能域 | Bun / Node.js / Erlang 生态 | CP 标准库 |
| :--- | :--- | :--- |
| **Actor 与信箱模型** | Erlang/OTP `spawn` / `send` / `receive` | `cap:actor` + `cap:beam` (`PID` / `SystemAction`) |
| **多核并行汇聚** | `Promise.all` / `Worker` | `cap:parallel` (`spawn` / `await join`) + `cap:worker` |
| **编译期反射** | TypeScript `reflect-metadata` (运行期) | `cap:reflect` (`TypeInfo` / `FieldInfo` / `MethodInfo`，零运行期开销) |
| **JSON / 序列化** | `JSON.parse` / `serde_json` | `cap:serde` (`JSON.parse` / `JSON.parseTyped`) |
| **嵌入式 SQLite** | `bun:sqlite` (`new Database(...)`) | `cap:sqlite` (`Database.init` / `exec` / `query_string` + RAII) |
| **内置单元测试** | `bun:test` (`test`, `expect().toBe()`) | `cap:test` (`test`, `expect_eq`) |

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/a2a

Official Google Agent-to-Agent (A2A) protocol Agent Card & JSON-RPC 2.0 task plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/a2a` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/a2a
```

```cp
import {
    A2aTaskState,
    A2aConfig,
    A2aTaskEntry,
    a2aBytesEql,
    a2aWriteByte,
    a2aWriteRaw,
    a2aWriteU64,
    a2aHexLower,
} from "@web/a2a";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `A2aTaskState` | `Submitted, Working, Completed, Canceled, Failed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `A2aConfig` | `class` | 字段: `agent_name: string, agent_description: string, agent_url: string, card_path: string, rpc_path: string, version: string` |
| `A2aTaskEntry` | `class` | 字段: `used: bool, id_buf: [A2A_TASK_ID_CAP]u8, id_len: usize, state: A2aTaskState, result_buf: [A2A_TASK_RESULT_CAP]u8, result_len: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `a2aBytesEql` | `(a: string, b: string): bool` | 调用 `a2aBytesEql` 并返回 `bool`。 |
| `a2aWriteByte` | `(out_buf: []u8, pos: &mut usize, b: u8): bool` | 调用 `a2aWriteByte` 并返回 `bool`。 |
| `a2aWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `a2aWriteRaw` 并返回 `bool`。 |
| `a2aWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `a2aWriteU64` 并返回 `bool`。 |
| `a2aHexLower` | `(n: u8): u8` | 调用 `a2aHexLower` 并返回 `u8`。 |
| `a2aWriteJsonEscaped` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `a2aWriteJsonEscaped` 并返回 `bool`。 |
| `a2aSkipWs` | `(s: string, start: usize): usize` | 调用 `a2aSkipWs` 并返回 `usize`。 |
| `a2aSkipJsonValue` | `(s: string, start: usize): usize` | 调用 `a2aSkipJsonValue` 并返回 `usize`。 |
| `a2aFindObjectFieldRaw` | `(obj: string, target_key: string): string?` | 调用 `a2aFindObjectFieldRaw` 并返回 `string?`。 |
| `a2aUnescapeJsonString` | `(raw: string, out_buf: []u8): usize?` | 调用 `a2aUnescapeJsonString` 并返回 `usize?`。 |
| `a2aSetTaskHandler` | `(handler: A2aTaskHandlerFn)` | 就地执行 `a2aSetTaskHandler` 操作。 |
| `a2aResetTasks` | `()` | 就地执行 `a2aResetTasks` 操作。 |
| `a2aFindTaskIndex` | `(task_id: string): usize?` | 调用 `a2aFindTaskIndex` 并返回 `usize?`。 |
| `a2aAllocTaskSlot` | `(task_id: string): usize?` | 调用 `a2aAllocTaskSlot` 并返回 `usize?`。 |
| `a2aJsonRpcError` | `(id_raw: string, code_str: string, message: string): Response` | 调用 `a2aJsonRpcError` 并返回 `Response`。 |
| `a2aTaskJsonResponse` | `(id_raw: string, idx: usize): Response` | 调用 `a2aTaskJsonResponse` 并返回 `Response`。 |
| `a2aCardHandler` | `(ctx: &mut Context): Response` | 调用 `a2aCardHandler` 并返回 `Response`。 |
| `a2aRpcHandler` | `(ctx: &mut Context): Response` | 调用 `a2aRpcHandler` 并返回 `Response`。 |
| `registerA2a` | `(app: &mut App, cfg: A2aConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { a2aBytesEql } from "@web/a2a";

pub fn main() {
    let result = a2aBytesEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

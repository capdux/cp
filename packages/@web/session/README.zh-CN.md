[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/session

Official zero-heap tamper-proof cookie session plugin for cap:web (pure-CP keyed MAC + fixed-slot KV store)

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/session` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/session
```

```cp
import {
    SessionConfig,
    SessionSlot,
    sessMonotonicNowSec,
    sessNowSec,
    sessFnv1a64Update,
    sessSplitMix64,
    sessWriteHexU64,
    sessComputeMacHex,
} from "@web/session";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `SessionConfig` | `class` | 字段: `secret: string, cookie_name: string, ttl_secs: u64, secure: bool, http_only: bool, same_site: string` |
| `SessionSlot` | `class` | 字段: `used: bool, id_buf: [SESS_MAX_ID_LEN]u8, id_len: usize, val_buf: [SESS_MAX_VAL_LEN]u8, val_len: usize, created_at: u64, +1 项` · 方法: `idSlice, valSlice, matchesId` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `sessMonotonicNowSec` | `(): u64` | 调用 `sessMonotonicNowSec` 并返回 `u64`。 |
| `sessNowSec` | `(): u64` | 调用 `sessNowSec` 并返回 `u64`。 |
| `sessFnv1a64Update` | `(seed: u64, data: &[]u8): u64` | 调用 `sessFnv1a64Update` 并返回 `u64`。 |
| `sessSplitMix64` | `(x: u64): u64` | 调用 `sessSplitMix64` 并返回 `u64`。 |
| `sessWriteHexU64` | `(val: u64, out_buf: []u8, offset: usize)` | 就地执行 `sessWriteHexU64` 操作。 |
| `sessComputeMacHex` | `(secret: string, id: string, out_hex: &mut [SESS_SIG_HEX_LEN]u8)` | 就地执行 `sessComputeMacHex` 操作。 |
| `sessConstTimeEql` | `(a: string, b: string): bool` | 调用 `sessConstTimeEql` 并返回 `bool`。 |
| `sessWriteRaw` | `(out_buf: []u8, pos: &mut usize, s: string): bool` | 调用 `sessWriteRaw` 并返回 `bool`。 |
| `sessWriteU64` | `(out_buf: []u8, pos: &mut usize, val: u64): bool` | 调用 `sessWriteU64` 并返回 `bool`。 |
| `sessVerifySignedCookie` | `(signed_cookie: string): string?` | 调用 `sessVerifySignedCookie` 并返回 `string?`。 |
| `sessFindValidSlotIndex` | `(id: string, now_sec: u64): usize?` | 调用 `sessFindValidSlotIndex` 并返回 `usize?`。 |
| `setSessionTestClock` | `(now_sec: u64)` | 写入或更新状态数据。 |
| `resetSessionStore` | `()` | 就地执行 `resetSessionStore` 操作。 |
| `sessionCreate` | `(user_val: string, now_sec: u64, out_cookie_val: []u8): usize?` | 调用 `sessionCreate` 并返回 `usize?`。 |
| `sessionGet` | `(signed_cookie: string, now_sec: u64): string?` | 调用 `sessionGet` 并返回 `string?`。 |
| `sessionDestroy` | `(signed_cookie: string): bool` | 调用 `sessionDestroy` 并返回 `bool`。 |
| `formatSessionSetCookie` | `(cfg: SessionConfig, signed_cookie: string, out_buf: []u8): usize?` | 通过 `formatSessionSetCookie` 编码或格式化输出，返回 `usize?`。 |
| `sessionHook` | `(ctx: &mut Context): bool` | 调用 `sessionHook` 并返回 `bool`。 |
| `createSessionPlugin` | `(cfg: SessionConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerSession` | `(app: &mut App, cfg: SessionConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { formatSessionSetCookie } from "@web/session";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatSessionSetCookie(/* cfg: SessionConfig */, "example", out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

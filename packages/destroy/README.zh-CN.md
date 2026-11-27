[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `destroy`

> **源包溯源**：`npm:destroy` ([stream-utils/destroy](https://github.com/stream-utils/destroy)) — 协议：MIT

纯 CP 实现的零堆分配、安全幂等流与文件描述符销毁库，完整覆盖 `ReadStream` 延迟打开关闭（`onOpen`）、轻量资源状态机 `DestroyState`、批量销毁辅助函数，并复用 `on-finished` 生命周期守卫。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `destroy`，无需改名：

```typescript
// TypeScript / Node.js (npm: destroy)
import destroy from "destroy";
destroy(readStream);
```

```cp
// CP（零堆分配，幂等流销毁）
import { DestroyableStream, destroy, isDestroyed } from "destroy";

let s = DestroyableStream.readFile(3, true);
let action = destroy(&mut s); // 1（立即销毁）
let done = isDestroyed(s);    // true
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "destroy": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `DestroyableStream` | `class`（`generic()`、`readFile(fd, fd_open)`、`zlib()`、`isDestroyed()`、`onOpen(fd)`） | 带 `on-finished` `StreamLifecycle` 的可销毁流模型 |
| `DestroyState` | `class`（`new(resource_id)`、`isDestroyed()`、`destroy()`、`destroyWithError(err_code)`、`reset()`） | 适用于任意资源的轻量幂等销毁状态机 |
| `destroy` / `destroyStream` | `(s: &mut DestroyableStream): u8` | 幂等销毁，返回动作码（`0`=无动作，`1`=已销毁，`2`=延迟关闭，`3`=重复销毁） |
| `destroyWithError` | `(s: &mut DestroyableStream, err_code: i32): u8` | 销毁流并将错误码 `err_code` 传播给生命周期监听者 |
| `isDestroyed` | `(s: DestroyableStream): bool` | 判断流是否已销毁或生命周期已结束 |
| `destroyAll` | `(streams: []DestroyableStream): usize` | 批量销毁流切片，返回本次新销毁的数量 |
| `destroyAllStates` | `(states: []DestroyState): usize` | 批量销毁 `DestroyState` 切片，返回本次新销毁的数量 |
| `isAnyDestroyed` | `(streams: &[]DestroyableStream): bool` | 切片中是否至少有一个流已销毁 |
| `areAllDestroyed` | `(streams: &[]DestroyableStream): bool` | 切片中是否所有流均已销毁 |

## 实战示例

### 1. 处理 `ReadStream` 延迟打开（Deferred Close）

```cp
import { DestroyableStream, destroyStream, destroyWithError } from "destroy";

// 文件描述符尚未打开（fd = -1, fd_open = false）
let rs = DestroyableStream.readFile(-1, false);

// 打开前调用销毁 -> 返回 2（DEFERRED_CLOSE）
let a1 = destroyStream(&mut rs);

// 随后系统打开 fd=7 时，onOpen() 自动立即关闭 -> 返回 1（DESTROYED）
let a2 = rs.onOpen(7);

// Zlib 流带错误码销毁
let zs = DestroyableStream.zlib();
let a3 = destroyWithError(&mut zs, -5);
```

### 2. 批量销毁与 `DestroyState` 状态追踪

```cp
import {
    DestroyableStream,
    DestroyState,
    destroyAll,
    destroyAllStates,
    isAnyDestroyed,
    areAllDestroyed,
} from "destroy";

let streams: [2]DestroyableStream = [
    DestroyableStream.generic(),
    DestroyableStream.zlib(),
];
let count = destroyAll(streams[0..2]);     // 2
let all = areAllDestroyed(&streams[0..2]); // true
let any = isAnyDestroyed(&streams[0..2]);  // true

let states: [2]DestroyState = [DestroyState.new(101), DestroyState.new(102)];
let closed = destroyAllStates(states[0..2]); // 2
```

## 注意事项与边界说明

- **严格幂等**：重复调用 `destroy` / `destroyStream` 会安全返回 `3`（`ALREADY_DESTROYED`），绝不会重复关闭文件描述符。
- **动作码约定**：`0`=无动作、`1`=立即销毁、`2`=延迟关闭（等待 `onOpen`）、`3`=已销毁。
- **零堆分配**：按值内嵌 `on-finished` 的 `StreamLifecycle`，无任何动态分配。

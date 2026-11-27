[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# fast-fifo

> **源包溯源**：吸收自 **npm** [`fast-fifo`](https://github.com/mafintosh/fast-fifo)（`v1.3.2` / `MIT`），CP 官方洁净室实现 `fast-fifo@1.3.2`。

纯 CP 实现的零堆分配 O(1) 环形缓冲区 FIFO 队列，提供 `i64` 与 `string` 两种专用队列类型。

## 核心亮点

- **零堆分配 2 的幂环形缓冲**：固定 256 槽位（`FIFO_CAP = 256`），采用位掩码回绕（`& 255`），无取模运算、无动态内存分配。
- **零栈清零初始化**：`newFifoI64()` 与 `newFifoStr()` 使用 `undefined` 初始化底层 `buf` 数组，彻底消除构造时的栈清零开销。
- **确定性 O(1) 延迟**：`push`、`shift`、`peek`、`len`、`isEmpty`、`clear` 全部为常数时间且缓存友好。

## 零成本迁移（TS → CP）

直接沿用原包名 `fast-fifo`，无需改名：

**TypeScript：**
```typescript
import FIFO from "fast-fifo";
const q = new FIFO<number>();
q.push(42);
const v = q.shift();
```

**CP：**
```typescript
import { FifoI64, FifoStr, newFifoI64, newFifoStr, FIFO_CAP } from "fast-fifo";
```

## 实战示例

### 1. 整数队列（`FifoI64`）

```typescript
import { newFifoI64 } from "fast-fifo";

export fn demoIntQueue(): i64 {
    let mut q = newFifoI64();
    _ = q.push(10);
    _ = q.push(20);
    _ = q.push(30);

    let mut sum: i64 = 0;
    while (!q.isEmpty()) {
        if (let v = q.shift()) {
            sum += v;
        }
    }
    return sum; // 60
}
```

### 2. 字符串任务队列（`FifoStr`）

```typescript
import { newFifoStr } from "fast-fifo";

export fn demoStrQueue(): usize {
    let mut q = newFifoStr();
    _ = q.push("job-1");
    _ = q.push("job-2");

    if (let next = q.peek()) {
        _ = next; // 查看队首 "job-1" 但不出队
    }
    let _first = q.shift();
    return q.len(); // 1
}
```

## 核心 API 速览

- **常量**：`FIFO_CAP: usize = 256`
- **构造器**：`newFifoI64(): FifoI64`、`newFifoStr(): FifoStr`
- **`FifoI64`**：`len()`、`isEmpty()`、`isFull()`、`push(val: i64): bool`、`shift(): i64?`、`peek(): i64?`、`clear()`
- **`FifoStr`**：`len()`、`isEmpty()`、`push(val: string): bool`、`shift(): string?`、`peek(): string?`、`clear()`

## 许可证

MIT

# `cap:circ_buf` — 纯 CP 泛型环形缓冲区 (Circular Ring Buffer)

## 概述

`cap:circ_buf`（亦支持 `cap.circ_buf` 动态挂载）实现泛型定容环形缓冲区 `RingBuffer(T)`，支持阻塞/非覆盖 `push`、覆盖最旧元素 `pushOverwrite`、双端弹出 `popFront`/`popBack`、逻辑下标随机访问 `get` 及线性导出 `toOwnedSlice`。

## 核心类型与 API

- `RingError`: `InvalidCapacity`、`BufferFull`、`BufferEmpty`、`IndexOutOfBounds`
- `RingBuffer(T)`: `init(alloc, cap_size)`、`deinit(&self)`、`push(&self, item)`、`pushOverwrite(&self, item)`、`popFront(&self)`、`popBack(&self)`、`peekFront(self)`、`peekBack(self)`、`get(self, idx)`、`toOwnedSlice(self, alloc)`、`clear(&self)`

## 示例

```cp
import { RingBuffer } from "cap:circ_buf";

let rb = try RingBuffer(i32).init(alloc, 4);
defer rb.deinit();
try rb.push(10);
_ = rb.pushOverwrite(20);
```

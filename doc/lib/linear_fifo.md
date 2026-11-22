# `cap:linear_fifo` — 零堆分配泛型环形 FIFO 缓冲区

> 参考来源：/data/dev/bun/src/collections/linear_fifo.cp | 吸收日期：2026-11-22

## 概述

`cap:linear_fifo` 吸收自 Bun 集合库（`src/collections/linear_fifo.cp`），提供纯 CP 零堆分配的固定容量泛型环形缓冲区 `StaticLinearFifo(T, capacity)`，适用于字节流缓冲、I/O 读写队列、网络帧重组与定长事件通道。

## 上游设计背景与行为契约

- **`head + count` 环形状态模型**：与传统 `head/tail` 双指针需要浪费 1 个槽位区分空/满不同，上游 `LinearFifo` 采用 `head`（读指针）+ `count`（当前可读元素数）表示环形窗口，可 100% 利用全部 `capacity` 个槽位。
- **自动处理环形回绕（Wrap-Around）**：批量写入 `writeSlice(src)` 与批量读取 `readSlice(dst)` 会自动将跨越缓冲区尾部边界的数据拆分为两段连续拷贝，对调用方完全透明。
- **前向回推与预览**：支持 `peekItem(idx)` 在不消费的前提下按逻辑偏移查看队列元素，以及 `unget(item)` 将已读元素回推至队首（适合词法分析器/协议解析器回退）。

## 导出符号与 API

### `StaticLinearFifo(comptime T: type, comptime capacity: usize)`

| 方法 | 说明 |
| :--- | :--- |
| `init()` | 初始化空 FIFO（`head = 0, count = 0`） |
| `clear()` | 清空队列并重置读写指针 |
| `isEmpty()` / `isFull()` | 判断队列是否为空或已满 |
| `readableLength()` / `writableLength()` | 返回当前可读元素数与剩余可写空间 |
| `writeItem(item)` | 写入单个元素（满时返回错误或失败状态） |
| `readItem()` | 弹出队首元素，空时返回 `null` |
| `peekItem(idx)` | 查看第 `idx` 个可读元素（不消费），越界返回 `null` |
| `unget(item)` | 将单个元素回推至队首（队列未满时生效） |
| `discard(n)` | 丢弃队首最多 `n` 个元素，推进读指针 |
| `writeSlice(src)` | 批量写入切片（自动处理环形回绕），返回实际写入元素数 |
| `readSlice(dst)` | 批量读取至目标切片（自动处理环形回绕），返回实际读取元素数 |

## 边界注意事项

1. **短读/短写语义**：`writeSlice` 与 `readSlice` 均按当前剩余可写/可读容量执行尽力拷贝并返回实际传输数量，不会越界或阻塞。
2. **2 的幂容量推荐**：选择 2 的幂（如 `64`、`256`、`4096`）作为 `capacity` 可让环形索引取模编译优化为按位与指令，获得最佳吞吐。
3. **单线程/单 Actor 独占**：`StaticLinearFifo` 为非原子无锁容器，跨线程共享请配合 `cap:sync` 互斥锁或改用 `cap:lockfree_queue`。

## 使用示例

```cp
import { StaticLinearFifo } from "cap:linear_fifo";

const ByteFifo = StaticLinearFifo(u8, 16);
let fifo = ByteFifo.init();
const written = fifo.writeSlice("hello world");
let out: [5]u8 = undefined;
const n = fifo.readSlice(out[0..]); // n == 5, out == "hello"
* = written;
* = n;
```

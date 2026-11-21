# `cap:linear_fifo`

纯 CP 零堆分配泛型环形缓冲区 FIFO（`StaticLinearFifo(T, capacity)`），吸收自 `/data/dev/bun/src/collections/linear_fifo.cp`。

## 导出符号

- `StaticLinearFifo(comptime T: type, comptime capacity: usize)`：
  - `init()` / `clear()` / `isEmpty()` / `isFull()` / `readableLength()` / `writableLength()`
  - `writeItem(item)` / `readItem()` / `peekItem(idx)` / `unget(item)` / `discard(n)`
  - `writeSlice(src)` / `readSlice(dst)`（自动处理环形回绕）

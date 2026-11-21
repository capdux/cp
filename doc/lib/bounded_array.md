# `cap:bounded_array` — 栈上定容动态数组 `BoundedArray<T, capacity>`

源自 Bun `collections/bounded_array.cp`，零堆分配的编译期固定容量动态数组，支持 `append`、`appendSlice`、`appendNTimes`、`pop`、`insert`、`orderedRemove`、`swapRemove`、`fromSlice`、`resize`、`clear` 与越界/溢出错误返回。

## 导出符号

- `BoundedArrayError`: `Overflow` / `OutOfBounds`
- `BoundedArray(comptime T: type, comptime buffer_capacity: usize)`

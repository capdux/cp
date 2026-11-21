# `cap:padding_checker` — 编译期结构体零未初始化填充校验器

> 吸收自 `/data/dev/bun/src/install/padding_checker.cp`，用于在二进制锁文件/快照序列化（`std.mem.asBytes` / `sliceAsBytes`）前于编译期校验结构体是否存在隐式未初始化填充字节（padding）或指针字段，杜绝内存垃圾字节泄漏。

## 核心导出 API

| 符号 | 说明 |
| :--- | :--- |
| `paddingBytes(comptime T)` | 返回结构体 `T` 中的隐式填充总字节数 |
| `hasPadding(comptime T)` | 递归检查 `T` 及嵌套字段是否存在隐式填充 |
| `hasPointers(comptime T)` | 递归检查 `T` 及嵌套字段是否包含指针（不适合直接跨进程二进制序列化） |
| `isSafeBinarySerializable(comptime T)` | 等价于 `!hasPointers(T) && !hasPadding(T)` |
| `assertNoUninitializedPadding(comptime T)` | 编译期强断言，若存在指针或隐式填充直接触发 `@compileError` |

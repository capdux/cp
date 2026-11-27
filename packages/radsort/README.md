[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# radsort

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:radsort` ([https://github.com/JakubValtar/radsort](https://github.com/JakubValtar/radsort))（原库名 `radsort` → CP 包名 `radsort`）

Zero-allocation O(N) 8-bit LSD radix sort for `u32`, `i32`, `u64`, `i64`, and IEEE-754 `f64` with automatic uniform-byte pass skipping for CP — clean-room MIT parity with Rust `radsort` and `voracious_radix_sort`.

零堆分配的 CP 线性时间 O(N) 无比较 8-bit 256 桶 LSD 基数排序引擎，自动跳过全同字节 pass，支持有符号整数符号位翻转与 IEEE-754 浮点保序双射映射（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add radsort
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `sortU32` / `sortI32` | `(data: []u32/i32, scratch: []u32/i32) -> bool` | 4-pass LSD 基数排序（自动跳过全同字节 pass） |
| `sortU64` / `sortI64` | `(data: []u64/i64, scratch: []u64/i64) -> bool` | 8-pass LSD 基数排序（完整覆盖 `i64::MIN..i64::MAX`） |
| `sortF64` | `(data: []f64, scratch: []f64) -> bool` | IEEE-754 保序双射 LSD 浮点排序 |
| `isSortedI64` / `isSortedU64` | `(data: []const i64/u64) -> bool` | 升序校验断言 |

## License

MIT

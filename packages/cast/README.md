[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:cast` ([https://github.com/japaric/cast.rs](https://github.com/japaric/cast.rs)) | 许可：`MIT OR Apache-2.0`

# `cast`

零堆分配纯 CP 受检基础数值类型转换库，彻底杜绝静默溢出、负数下溢与浮点 `NaN`/`Inf` 截断，对标 crates.io **#443** `cast`（真实总下载 **3.35 亿+**）。

## 核心 API

- 整数受检转换：`u8FromI64` / `u8FromU64` / `u16FromI64` / `u32FromI64` / `u32FromU64` / `i32FromU64` / `i32FromI64` / `u64FromI64` / `i64FromU64` / `usizeFromI64`
- 浮点受检转换：`u8FromF64` / `u32FromF64` / `i32FromF64` / `checkF64ToI32`

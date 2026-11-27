[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# ee-first

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ee-first` ([https://github.com/jonathanong/ee-first](https://github.com/jonathanong/ee-first))

零堆分配的纯 CP 多事件源首个事件竞态监听器，Clean-Room MIT 实现对标 npm Top 30 包 `ee-first` v1（周下载 1.6 亿+）。

## 特性

- **零堆 16 槽位竞态**：`FirstRace` 使用固定 `[16]EventWatch` 栈数组注册多组 `(emitter_id, event_name)` 监听
- **首个命中自动清理**：`emit` 命中首个活跃监听后立即记录获胜事件（`FirstResult`），并自动取消其余全部活跃监听
- **手动取消与状态查询**：支持 `cancel()` 手动释放剩余监听，以及 `activeCount()`、`isSettled()`、`winner()` 零开销查询

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jonathanong/ee-first`](https://github.com/jonathanong/ee-first)。

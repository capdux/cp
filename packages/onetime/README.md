[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/onetime

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:onetime` ([https://github.com/sindresorhus/onetime](https://github.com/sindresorhus/onetime))

零堆分配的 CP 原生单次执行守卫与调用计数器，Clean-Room MIT 实现对标 npm 顶流包 `onetime`（1.5 亿+/周下载）。

## 特性

- `OnceGuard`：确保初始化逻辑仅执行一次，缓存首次返回值并记录 `callCount()`
- `OnceGuard.newStrict()`：严格模式（对标 `onetime(fn, { throw: true })`），二次调用返回 `ONCE_ALREADY_CALLED`
- `OnceRegistry`：支持最多 64 个独立键的一次性执行注册表

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`sindresorhus/onetime`](https://github.com/sindresorhus/onetime)。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:tapable` ([https://github.com/webpack/tapable](https://github.com/webpack/tapable)) | 许可：`MIT`

# `tapable`

零堆分配纯 CP 插件钩子引擎，完整对标 npm **#242** `tapable` v2（真实周下载 **1.95 亿/周**，Webpack / Rspack 插件架构核心依赖）。

## 核心 API

- `TapEntry`：`name`, `stage`, `handler_id`, `enabled`
- `SyncHook`：按 `stage` 升序与 `before` 插队顺序依次触发全部启用回调（最多 32 个 tap）
- `SyncBailHook`：首个返回非 null 结果的回调立即短路返回
- `SyncWaterfallHook`：将前一个回调的返回值依次传递给下一个回调
- `SyncLoopHook`：任意回调返回重启信号时从头重新循环直至全部收敛（含 `max_rounds` 防死循环保护）
- 统一方法：`.tap`、`.tapStage`、`.tapBefore`、`.setEnabled`、`.isUsed`、`.tapCount`、`.getOrderedHandlers` 及 `.call`

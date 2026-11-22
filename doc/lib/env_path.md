# `cap:env_path` — 跨平台 `PATH` 环境变量构建器与零分配迭代器

> 参考来源：/data/dev/bun/src/paths/EnvPath.cp | 吸收日期：2026-11-22

## 概述

`cap:env_path` 吸收自 Bun 路径子系统（`src/paths/EnvPath.cp`），用于跨平台构建、规范化与遍历 `PATH` 类分隔路径列表。模块提供带 `~init()` 确定性 RAII 释放的 `EnvPath` 构建器以及零堆分配的 `EnvPathIterator` 迭代器。

## 上游行为契约与 CP 增强

- **分隔符与尾部斜杠自动规范化（吸收自 Bun `EnvPath.cp`）**：
  - 追加路径段时，自动裁剪输入首尾冗余的路径列表分隔符（POSIX `:` / Windows `;`）以及路径尾部多余的斜杠（`/`），避免生成 `::` 空段或重复分隔符；
  - 若裁剪后的路径为空串，自动静默跳过而不写入无效分隔符；
  - 仅在缓冲区非空时才在项之间插入分隔符，确保输出首尾始终干净。
- **CP 生产级扩展**：
  - **确定性 RAII**：`EnvPath` 实现 `~init()` 析构函数，作用域结束自动释放底层缓冲；
  - **去重与前置能力**：在 Bun 原生 `append` 基础上新增 `appendUnique`（避免重复注入同一目录）、`prepend`（将高优先级工具链目录置于最前）、`contains`（精确段查询）、`clear` 与 `initWithDelimiter`（支持自定义分隔符，便于跨平台测试）；
  - **零分配遍历**：提供 `EnvPathIterator` 直接在只读切片上迭代每个有效路径段，自动跳过连续分隔符产生的空段。

## 导出符号与 API

### 常量

- `DEFAULT_DELIMITER: u8` — 平台默认 `PATH` 分隔符（POSIX 为 `':'`，Windows 为 `';'`）。

### `EnvPathIterator`

- `static fn init(raw: []const u8, delimiter: u8): EnvPathIterator`
- `fn next(*_): ([]const u8)?` — 零堆分配返回下一个非空路径段，遍历结束返回 `null`。

### `EnvPath`（含 `~init()` RAII）

| 方法 | 说明 |
| :--- | :--- |
| `init(allocator)` / `initWithDelimiter(allocator, delimiter)` | 使用默认或自定义分隔符初始化构建器 |
| `slice()` / `len()` / `isEmpty()` | 获取当前构建的 `PATH` 切片视图与状态 |
| `iterator()` | 返回遍历当前所有路径段的 `EnvPathIterator` |
| `contains(path)` | 判断指定目录是否已存在（按规范化路径段精确匹配） |
| `append(path)` | 规范化后追加到末尾 |
| `appendUnique(path)` | 若 `contains(path)` 为 `false` 则追加，防止重复项 |
| `prepend(path)` | 规范化后插入到最前面（最高查找优先级） |
| `clear()` | 清空已构建内容但保留底层容量 |

## 边界注意事项

1. **空项过滤**：传入 `""`、`":::"` 或仅含分隔符的字符串时，`append` / `prepend` 均安全忽略。
2. **根目录保留**：对于 POSIX 根目录 `"/"` 等特殊路径，规范化逻辑不会将其误裁剪为空串。
3. **切片生命周期**：`EnvPath.slice()` 与 `iterator()` 产出的切片指向内部缓冲，在发生扩容（`append` / `prepend`）后旧切片可能失效。

## 使用示例

```cp
import std from "std";
import { EnvPath, EnvPathIterator, DEFAULT_DELIMITER } from "cap:env_path";

fn buildPath(alloc: std.mem.Allocator) {
    let ep = EnvPath.initWithDelimiter(alloc, ':');
    * = ep.append("/usr/local/bin/");
    * = ep.appendUnique("/usr/local/bin"); // 已存在，自动去重
    * = ep.prepend("/opt/cp/bin");
    // ep.slice() == "/opt/cp/bin:/usr/local/bin"

    let it = EnvPathIterator.init(ep.slice(), DEFAULT_DELIMITER);
    while (seg := it.next()) {
        * = seg;
    }
}
```

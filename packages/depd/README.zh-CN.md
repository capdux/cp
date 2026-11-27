[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# depd

面向 CP 语言的零堆分配弃用警告格式化器与去重追踪器，对齐 npm [`depd`](https://github.com/dougwilson/nodejs-depd)（周下载 1.5 亿+）。

## 源包对齐说明

- **上游源包**：npm `depd`（`dougwilson/nodejs-depd`，MIT）
- **CP 实现**：`depd` v1.0.0，纯 CP 净室 MIT 实现，复用 `@sindresorhus/fnv1a` 进行 64 位 FNV-1a 去重哈希
- **架构特点**：全程零堆分配，所有格式化输出写入调用方提供的 `[]u8` 缓冲区，去重表为固定 64 槽栈数组（`MAX_SEEN_HASHES = 64`）。

## 安装与导入

```bash
cap add depd
```

```cp
import {
    MAX_SEEN_HASHES,
    DepdOptions,
    Deprecator,
    isNamespaceSilenced,
    isNamespaceTraced,
    formatDeprecationLen,
    formatDeprecation,
    formatDeprecationWithTrace,
    depdHashKey,
} from "depd";
```

## 核心 API

### 类型与常量

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `MAX_SEEN_HASHES` | `const MAX_SEEN_HASHES: usize`（`64`） | 单个 `Deprecator` 实例最多缓存的去重哈希数量 |
| `DepdOptions` | `struct { no_deprecation: bool, trace_deprecation: bool }` | 配置项结构体；可通过 `DepdOptions.defaults()` 获取默认值（全 `false`） |
| `Deprecator` | `struct { namespace, silenced, no_deprecation, trace_deprecation, seen_hashes: [64]u64, seen_count, emitted_count }` | 带去重状态和静默/堆栈开关的命名空间弃用告警器 |

### `Deprecator` 方法

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Deprecator.new` | `fn new(namespace: string): Deprecator` | 使用默认选项创建 `Deprecator` |
| `Deprecator.withOptions` | `fn withOptions(namespace: string, opts: DepdOptions): Deprecator` | 使用指定 `DepdOptions` 创建 `Deprecator` |
| `setSilenced` | `fn setSilenced(*_, flag: bool)` | 设置运行时静默开关（需可变接收者） |
| `setNoDeprecation` | `fn setNoDeprecation(*_, flag: bool)` | 设置 `NO_DEPRECATION` 屏蔽标志 |
| `setTraceDeprecation` | `fn setTraceDeprecation(*_, flag: bool)` | 设置 `TRACE_DEPRECATION` 堆栈追踪开关 |
| `isSilenced` | `fn isSilenced(): bool` | 若 `silenced \|\| no_deprecation` 为真则返回 `true` |
| `isTraceEnabled` | `fn isTraceEnabled(): bool` | 返回是否开启堆栈追踪输出 |
| `seenCount` | `fn seenCount(): usize` | 已记录的唯一警告条数（上限 `64`） |
| `hasWarned` | `fn hasWarned(msg: string): bool` | 只读查询 `msg` 是否已在该命名空间下输出过 |
| `warnOnce` | `fn warnOnce(*_, msg: string, site: string, out: []u8): usize` | 首次触发时格式化写入 `out` 并返回写入字节数；静默或重复返回 `0` |
| `warnWithTrace` | `fn warnWithTrace(*_, msg: string, site: string, trace: string, out: []u8): usize` | 去重输出，并在开启 `trace_deprecation` 时追加 `\n<trace>` |
| `deprecate` | `fn deprecate(*_, msg: string, site: string, out: []u8): usize` | `warnOnce` 的语义别名 |
| `deprecateFn` | `fn deprecateFn(*_, fn_name: string, msg: string, site: string, out: []u8): usize` | 函数弃用告警（当 `msg` 为空时自动使用 `fn_name`） |
| `reset` | `fn reset(*_)` | 清空去重表并将 `seen_count`、`emitted_count` 归零 |

### 独立函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isNamespaceSilenced` | `fn isNamespaceSilenced(namespace: string, no_deprecation_env: string): bool` | 按逗号分隔列表匹配 `NO_DEPRECATION`（支持 `*` 通配符） |
| `isNamespaceTraced` | `fn isNamespaceTraced(namespace: string, trace_deprecation_env: string): bool` | 按逗号分隔列表匹配 `TRACE_DEPRECATION`（支持 `*` 通配符） |
| `formatDeprecationLen` | `fn formatDeprecationLen(namespace: string, msg: string, site: string): usize` | 精确计算 `<namespace> deprecated <msg>[ at <site>]` 的字节长度 |
| `formatDeprecation` | `fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize` | 无状态格式化写入 `out`，返回实际写入字节数 |
| `formatDeprecationWithTrace` | `fn formatDeprecationWithTrace(namespace: string, msg: string, site: string, trace: string, include_trace: bool, out: []u8): usize` | 无状态格式化，按 `include_trace` 决定是否追加 `\n<trace>` |
| `depdHashKey` | `fn depdHashKey(ns: string, msg: string): u64` | 计算 `(ns, msg)` 的 64 位 FNV-1a 哈希键 |
| `copySlice` | `fn copySlice(dst: []u8, pos: usize, src: string): usize` | 带边界保护的切片拷贝辅助函数，返回新偏移 |

## 使用示例

### 1. 去重输出弃用警告

```cp
import { Deprecator } from "depd";
import { print } from "cap:io";

pub fn main() {
    let dep = Deprecator.new("my-http");
    let buf: [256]u8 = [0; 256];

    const n1 = dep.deprecate("req.host is deprecated; use req.hostname", "router.cp:42", buf[0..256]);
    if (n1 > 0) {
        print(buf[0..n1] as string);
        // "my-http deprecated req.host is deprecated; use req.hostname at router.cp:42"
    }

    // 第二次相同告警将被自动去重，返回 0
    const n2 = dep.deprecate("req.host is deprecated; use req.hostname", "router.cp:42", buf[0..256]);
    // n2 == 0, dep.seenCount() == 1
}
```

### 2. 基于环境变量列表的静默与堆栈配置

```cp
import { DepdOptions, Deprecator, isNamespaceSilenced, isNamespaceTraced } from "depd";

pub fn initDeprecator(no_dep_env: string, trace_dep_env: string): Deprecator {
    const opts = DepdOptions {
        no_deprecation: isNamespaceSilenced("express", no_dep_env),
        trace_deprecation: isNamespaceTraced("express", trace_dep_env),
    };
    return Deprecator.withOptions("express", opts);
}
```

### 3. 预计算长度与无状态格式化

```cp
import { formatDeprecationLen, formatDeprecation } from "depd";

pub fn formatOne(out: []u8): usize {
    const needed = formatDeprecationLen("body-parser", "extended option required", "app.cp:18");
    if (needed > out.len) {
        return 0;
    }
    return formatDeprecation("body-parser", "extended option required", "app.cp:18", out);
}
```

## 与源包差异与安全亮点

- **显式缓冲区输出**：不直接操作 `stderr` 或动态劫持函数属性，所有消息写入调用方 `[]u8` 缓冲区，完全可控、无隐藏副作用。
- **纯函数环境匹配**：`isNamespaceSilenced` / `isNamespaceTraced` 显式接收环境变量字符串，便于单元测试与无全局状态的并发运行。
- **零堆去重**：固定 `[64]u64` 栈上哈希表 + 边界安全拷贝，无堆分配与越界风险。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# onetime

Ensure an initialization or action runs only once with call-count tracking (clean-room CP implementation of npm onetime)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:onetime` ([https://github.com/sindresorhus/onetime](https://github.com/sindresorhus/onetime))

| 元信息 | 值 |
| --- | --- |
| 包名 | `onetime` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add onetime
```

```cp
import { OnceCallResult, OnceGuard, OnceRegistry } from "onetime";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `OnceCallResult` | `class` | 字段: `status: i32, executed_now: bool, value: i64, call_count: u64` |
| `OnceGuard` | `class` | 字段: `called: bool, strict: bool, call_count: u64, cached_value: i64` · 方法: `new, newStrict, isCalled, callCount, cachedValue, runWithValue, +2 项` |
| `OnceRegistry` | `class` | 字段: `keys: [64]u64, values: [64]i64, counts: [64]u64, len: usize` · 方法: `new, size, has, callCountOf, runForKey` |

## 最小调用示例

```cp
import { OnceCallResult } from "onetime";

pub fn main() {
    // Refer to src/index.d.cp for OnceCallResult fields and methods
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

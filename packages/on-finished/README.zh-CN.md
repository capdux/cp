[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# on-finished

Zero-heap HTTP message/stream completion tracker with once-only finish/error/close lifecycle dispatch (npm on-finished equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:on-finished` ([https://github.com/jshttp/on-finished](https://github.com/jshttp/on-finished))

| 元信息 | 值 |
| --- | --- |
| 包名 | `on-finished` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add on-finished
```

```cp
import { ListenerSlot, StreamLifecycle, isFinished } from "on-finished";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ListenerSlot` | `class` | 字段: `id: u32, fired: bool, reason: u8, err_code: i32` |
| `StreamLifecycle` | `class` | 字段: `readable_ended: bool, writable_finished: bool, socket_closed: bool, errored: bool, err_code: i32, finish_reason: u8, +3 项` · 方法: `new, isFinished, isReadableFinished, isWritableFinished, onFinished, dispatchAll, +5 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isFinished` | `(lc: StreamLifecycle): bool` | 执行 `isFinished` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isFinished } from "on-finished";

pub fn main() {
    let result = isFinished(/* lc: StreamLifecycle */);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

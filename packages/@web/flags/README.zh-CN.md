[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @web/flags

Official deterministic FNV-1a Feature Flags & percentage rollout plugin for cap:web

| 元信息 | 值 |
| --- | --- |
| 包名 | `@web/flags` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @web/flags
```

```cp
import {
    FlagsConfig,
    FeatureFlag,
    flgStrEql,
    flgInList,
    flagBucket,
    flagReset,
    flagDefine,
    flagEval,
} from "@web/flags";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FlagsConfig` | `class` | 字段: `status_path: string, user_header: string` |
| `FeatureFlag` | `class` | 字段: `name: string, enabled: bool, rollout_pct: u8, allow_users: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `flgStrEql` | `(a: string, b: string): bool` | 调用 `flgStrEql` 并返回 `bool`。 |
| `flgInList` | `(list: string, target: string): bool` | 调用 `flgInList` 并返回 `bool`。 |
| `flagBucket` | `(flag_name: string, user_key: string): u8` | 调用 `flagBucket` 并返回 `u8`。 |
| `flagReset` | `()` | 就地执行 `flagReset` 操作。 |
| `flagDefine` | `(name: string, enabled: bool, rollout_pct: u8, allow_users: string): bool` | 调用 `flagDefine` 并返回 `bool`。 |
| `flagEval` | `(flag_name: string, user_key: string): bool` | 调用 `flagEval` 并返回 `bool`。 |
| `flgWriteRaw` | `(dst: []u8, pos: &mut usize, s: string): bool` | 调用 `flgWriteRaw` 并返回 `bool`。 |
| `flgWriteU8` | `(dst: []u8, pos: &mut usize, v: u8): bool` | 调用 `flgWriteU8` 并返回 `bool`。 |
| `handleFlagsStatus` | `(ctx: &mut Context): Response` | 调用 `handleFlagsStatus` 并返回 `Response`。 |
| `createFlagsPlugin` | `(cfg: FlagsConfig): Plugin` | 创建或初始化实例，返回 `Plugin`。 |
| `registerFlags` | `(app: &mut App, cfg: FlagsConfig): &mut App` | 将插件或中间件注册到目标应用实例。 |

## 最小调用示例

```cp
import { flgStrEql } from "@web/flags";

pub fn main() {
    let result = flgStrEql("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

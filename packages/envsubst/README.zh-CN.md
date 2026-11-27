[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# envsubst

Zero-allocation POSIX / Docker / K8s environment variable template expansion engine (dotenv-expand / envsub / Rust shellexpand parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dotenv-expand` ([https://github.com/motdotla/dotenv-expand](https://github.com/motdotla/dotenv-expand))（原库名 `dotenv-expand` → CP 包名 `envsubst`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `envsubst` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add envsubst
```

```cp
import {
    ExpandError,
    EnvMap,
    ExpandResult,
    isVarStart,
    isVarCont,
    expandImpl,
    expandEnv,
    expandEnvStr,
} from "envsubst";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `ExpandError` | `None, UnclosedBrace, MissingRequired, BufferOverflow` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `EnvMap` | `class` | 字段: `keys: [MAX_ENV_VARS]string, vals: [MAX_ENV_VARS]string, len: usize` · 方法: `init, set, get, has` |
| `ExpandResult` | `class` | 字段: `ok: bool, err: ExpandError, len: usize, missing_var: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isVarStart` | `(c: u8): bool` | 执行 `isVarStart` 条件判定，返回 `bool`。 |
| `isVarCont` | `(c: u8): bool` | 执行 `isVarCont` 条件判定，返回 `bool`。 |
| `expandImpl` | `(tpl: string, env: EnvMap, out: []u8, start_out: usize, depth: u8): ExpandResult` | 调用 `expandImpl` 并返回 `ExpandResult`。 |
| `expandEnv` | `(tpl: string, env: EnvMap, out: []u8): ExpandResult` | 调用 `expandEnv` 并返回 `ExpandResult`。 |
| `expandEnvStr` | `(tpl: string, env: EnvMap, out: []u8): string` | 调用 `expandEnvStr` 并返回 `string`。 |
| `hasEnvVars` | `(tpl: string): bool` | 执行 `hasEnvVars` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { expandEnv } from "envsubst";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = expandEnv("example", /* env: EnvMap */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

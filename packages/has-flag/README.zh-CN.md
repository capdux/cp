[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# has-flag

Check if CLI argv contains a specific flag (clean-room CP implementation of npm has-flag)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:has-flag` ([https://github.com/sindresorhus/has-flag](https://github.com/sindresorhus/has-flag))

| 元信息 | 值 |
| --- | --- |
| 包名 | `has-flag` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add has-flag
```

```cp
import {
    hfEq,
    hfStartsWith,
    hfMatchesFlag,
    hfComputePrefixLen,
    findFlagIndex,
    hasFlag,
    getFlagValue,
    isFlagEnabled,
} from "has-flag";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hfEq` | `(a: string, b: string): bool` | 调用 `hfEq` 并返回 `bool`。 |
| `hfStartsWith` | `(s: string, prefix: string): bool` | 调用 `hfStartsWith` 并返回 `bool`。 |
| `hfMatchesFlag` | `(arg: string, flag: string, prefix_len: usize): bool` | 调用 `hfMatchesFlag` 并返回 `bool`。 |
| `hfComputePrefixLen` | `(flag: string): usize` | 调用 `hfComputePrefixLen` 并返回 `usize`。 |
| `findFlagIndex` | `(flag: string, argv: &[]string): i64` | 查询或读取目标数据，返回 `i64`。 |
| `hasFlag` | `(flag: string, argv: &[]string): bool` | 执行 `hasFlag` 条件判定，返回 `bool`。 |
| `getFlagValue` | `(flag: string, argv: &[]string, out: []u8): i64` | 查询或读取目标数据，返回 `i64`。 |
| `isFlagEnabled` | `(flag: string, argv: &[]string, default_val: bool): bool` | 执行 `isFlagEnabled` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { getFlagValue } from "has-flag";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = getFlagValue("example", /* argv: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

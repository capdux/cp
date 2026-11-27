[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-glob

Zero-allocation glob pattern detector reusing is-extglob for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:is-glob` ([https://github.com/micromatch/is-glob](https://github.com/micromatch/is-glob))

| 元信息 | 值 |
| --- | --- |
| 包名 | `is-glob` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add is-glob
```

```cp
import {
    hasClosedBracket,
    hasClosedBrace,
    hasUnescapedWildcard,
    isGlob,
    isGlobDefault,
} from "is-glob";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hasClosedBracket` | `(str: string, open_idx: usize): bool` | 执行 `hasClosedBracket` 条件判定，返回 `bool`。 |
| `hasClosedBrace` | `(str: string, open_idx: usize): bool` | 执行 `hasClosedBrace` 条件判定，返回 `bool`。 |
| `hasUnescapedWildcard` | `(str: string): bool` | 执行 `hasUnescapedWildcard` 条件判定，返回 `bool`。 |
| `isGlob` | `(str: string, strict: bool): bool` | 执行 `isGlob` 条件判定，返回 `bool`。 |
| `isGlobDefault` | `(str: string): bool` | 执行 `isGlobDefault` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isGlob } from "is-glob";

pub fn main() {
    let result = isGlob("example", false);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# yn

Zero-allocation parser for yes/no boolean strings (y/yes/true/1/on/n/no/false/0/off) with lenient mode

| 元信息 | 值 |
| --- | --- |
| 包名 | `yn` |
| 版本 | `5.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add yn
```

```cp
import { yn, ynLenient, ynDefault } from "yn";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `yn` | `(input: string): bool?` | 调用 `yn` 并返回 `bool?`。 |
| `ynLenient` | `(input: string): bool?` | 调用 `ynLenient` 并返回 `bool?`。 |
| `ynDefault` | `(input: string, default_val: bool): bool` | 调用 `ynDefault` 并返回 `bool`。 |

## 最小调用示例

```cp
import { yn } from "yn";

pub fn main() {
    let result = yn("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

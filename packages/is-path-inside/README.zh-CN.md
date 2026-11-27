[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-path-inside

Zero-allocation path traversal guard checking whether child_path is strictly inside parent_path

| 元信息 | 值 |
| --- | --- |
| 包名 | `is-path-inside` |
| 版本 | `4.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add is-path-inside
```

```cp
import { ipiIsSep, ipiNormalize, isPathInside, isPathInsideOrEqual } from "is-path-inside";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ipiIsSep` | `(b: u8): bool` | 调用 `ipiIsSep` 并返回 `bool`。 |
| `ipiNormalize` | `(input: string, out: []u8): usize` | 调用 `ipiNormalize` 并返回 `usize`。 |
| `isPathInside` | `(child_path: string, parent_path: string): bool` | 执行 `isPathInside` 条件判定，返回 `bool`。 |
| `isPathInsideOrEqual` | `(child_path: string, parent_path: string): bool` | 执行 `isPathInsideOrEqual` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isPathInside } from "is-path-inside";

pub fn main() {
    let result = isPathInside("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

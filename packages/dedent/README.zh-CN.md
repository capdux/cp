[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dedent

Zero-heap common leading indentation detector and dedent engine for multiline strings (compatible with npm strip-indent, dedent & Rust indoc)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:dedent` ([https://github.com/dmnd/dedent](https://github.com/dmnd/dedent))（原库名 `dedent` → CP 包名 `dedent`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `dedent` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add dedent
```

```cp
import { isHorizSpace, minIndent, stripIndent, indentLines } from "dedent";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isHorizSpace` | `(c: u8): bool` | 执行 `isHorizSpace` 条件判定，返回 `bool`。 |
| `minIndent` | `(input: string): usize` | 调用 `minIndent` 并返回 `usize`。 |
| `stripIndent` | `(input: string, out: []u8): usize` | 调用 `stripIndent` 并返回 `usize`。 |
| `indentLines` | `(input: string, spaces: usize, out: []u8): usize` | 调用 `indentLines` 并返回 `usize`。 |

## 最小调用示例

```cp
import { minIndent } from "dedent";

pub fn main() {
    let result = minIndent("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

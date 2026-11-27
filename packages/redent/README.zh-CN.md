[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# redent

Zero-allocation strip-and-reindent multiline string utility (npm redent parity) composing strip-indent and indent-string for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:redent` ([https://github.com/sindresorhus/redent](https://github.com/sindresorhus/redent))

| 元信息 | 值 |
| --- | --- |
| 包名 | `redent` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add redent
```

```cp
import { redentWithIndent, redent } from "redent";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `redentWithIndent` | `(s: string, count: usize, indent: string, include_empty_lines: bool, out: []u8): usize` | 调用 `redentWithIndent` 并返回 `usize`。 |
| `redent` | `(s: string, count: usize, out: []u8): usize` | 调用 `redent` 并返回 `usize`。 |

## 最小调用示例

```cp
import { redent } from "redent";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = redent("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

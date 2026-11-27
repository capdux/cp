[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# metaphone

Zero-allocation Lawrence Philips Metaphone English phonetic encoding algorithm in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:metaphone` ([https://github.com/words/metaphone](https://github.com/words/metaphone))

| 元信息 | 值 |
| --- | --- |
| 包名 | `metaphone` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add metaphone
```

```cp
import { isVowel, metaphone, soundsLike } from "metaphone";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isVowel` | `(b: u8): bool` | 执行 `isVowel` 条件判定，返回 `bool`。 |
| `metaphone` | `(word: string, max_len: usize, out: []u8): usize` | 调用 `metaphone` 并返回 `usize`。 |
| `soundsLike` | `(a: string, b: string): bool` | 调用 `soundsLike` 并返回 `bool`。 |

## 最小调用示例

```cp
import { metaphone } from "metaphone";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = metaphone("example", 0, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

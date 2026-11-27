[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ccount

Zero-allocation character/substring occurrence counter and longest consecutive streak finder

| 元信息 | 值 |
| --- | --- |
| 包名 | `ccount` |
| 版本 | `2.0.1` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ccount
```

```cp
import { ccountByte, ccount, longestStreakByte, longestStreak } from "ccount";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `ccountByte` | `(value: string, ch: u8): usize` | 调用 `ccountByte` 并返回 `usize`。 |
| `ccount` | `(value: string, needle: string): usize` | 调用 `ccount` 并返回 `usize`。 |
| `longestStreakByte` | `(value: string, ch: u8): usize` | 调用 `longestStreakByte` 并返回 `usize`。 |
| `longestStreak` | `(value: string, needle: string): usize` | 调用 `longestStreak` 并返回 `usize`。 |

## 最小调用示例

```cp
import { ccountByte } from "ccount";

pub fn main() {
    let result = ccountByte("example", 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

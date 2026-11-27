[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# toml_writer

Zero-heap TOML 1.0 serializer and formatter (bare/quoted keys, escaped strings, tables, array-of-tables, inline arrays) inspired by Rust toml_writer

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:toml_writer` ([https://github.com/toml-rs/toml](https://github.com/toml-rs/toml))（原库名 `toml_writer` → CP 包名 `toml_writer`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `toml_writer` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add toml_writer
```

```cp
import { TomlWriter, isBareKey } from "toml_writer";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `TomlWriter` | `class` | 字段: `out: []u8, pos: usize, overflow: bool` · 方法: `init, asSlice, putByte, putRaw, writeQuotedString, writeKey, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isBareKey` | `(k: string): bool` | 执行 `isBareKey` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isBareKey } from "toml_writer";

pub fn main() {
    let result = isBareKey("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

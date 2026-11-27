[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# ucd-trie

Pure CP zero-allocation compressed two-level bitmap Trie for O(1) Unicode codepoint membership queries (ucd-trie parity)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:ucd-trie` ([https://github.com/BurntSushi/ucd-generate](https://github.com/BurntSushi/ucd-generate))

| 元信息 | 值 |
| --- | --- |
| 包名 | `ucd-trie` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add ucd-trie
```

```cp
import { UcdTrie, utPopcount64, utAllOnes64 } from "ucd-trie";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UcdTrie` | `class` | 字段: `max_cp: u32, chunk_count: usize` · 方法: `new, init, insertCodepoint, insertRange, contains, countMembers, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `utPopcount64` | `(x: u64): u64` | 调用 `utPopcount64` 并返回 `u64`。 |
| `utAllOnes64` | `(): u64` | 调用 `utAllOnes64` 并返回 `u64`。 |

## 最小调用示例

```cp
import { utPopcount64 } from "ucd-trie";

pub fn main() {
    let result = utPopcount64(0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

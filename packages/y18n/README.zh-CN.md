[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# y18n

Zero-allocation internationalization (i18n) catalog and printf-style formatter (y18n parity) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:y18n` ([https://github.com/yargs/y18n](https://github.com/yargs/y18n))

| 元信息 | 值 |
| --- | --- |
| 包名 | `y18n` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add y18n
```

```cp
import { Y18nEntry, Y18nCatalog, y18WriteByte, formatPrintf } from "y18n";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Y18nEntry` | `class` | 字段: `key: string, singular: string, plural: string, has_plural: bool` |
| `Y18nCatalog` | `class` | 字段: `locale: string, count: usize, entries: [64]Y18nEntry` · 方法: `new, setLocale, getLocale, clear, findIndex, hasEntry, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `y18WriteByte` | `(out: []u8, pos: usize, b: u8): usize` | 调用 `y18WriteByte` 并返回 `usize`。 |
| `formatPrintf` | `(tmpl: string, count: i64, has_count: bool, args: &[]string, out: []u8): usize` | 通过 `formatPrintf` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { formatPrintf } from "y18n";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = formatPrintf("example", 0, false, /* args: &[]string */, out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-extglob

面向 CP 语言的零分配 Bash / Micromatch 扩展通配符（`@(...)`、`!(...)`、`*(...)`、`+(...)`、`?(...)`）检测与区间定位库，对齐 npm [`is-extglob`](https://github.com/jonschlinkert/is-extglob)。

## 源包对齐说明

- **上游源包**：npm `is-extglob`（`jonschlinkert/is-extglob`，MIT）
- **CP 实现**：`is-extglob` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：零堆分配、单趟括号平衡扫描（自动处理 `\` 转义），在布尔判断基础上扩展了 `ExtglobSpan` 字节区间与 `ExtglobKind` 操作符类型解析。

## 安装与导入

```bash
cap add is-extglob
```

```cp
import {
    ExtglobKind,
    ExtglobSpan,
    isExtglobPrefix,
    extglobKindFromByte,
    extglobPrefixByte,
    matchExtglobEnd,
    findExtglobFrom,
    findExtglob,
    findExtglobStart,
    isExtglob,
    hasNestedExtglob,
    isNegatedExtglob,
    countExtglobs,
} from "is-extglob";
```

## 核心 API

### 类型定义

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `ExtglobKind` | `enum { Optional, ZeroOrMore, OneOrMore, ExactlyOne, Negated }` | 扩展通配符操作符类型（`?`、`*`、`+`、`@`、`!`） |
| `ExtglobSpan` | `struct { kind: ExtglobKind, start: usize, end: usize }` | 匹配的字节范围 `str[start..end]`（右开区间）及操作符类型 |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isExtglob` | `fn isExtglob(str: string): bool` | 判断字符串中是否包含合法且未转义的 extglob 表达式 |
| `findExtglob` | `fn findExtglob(str: string): ExtglobSpan?` | 查找首个完整 extglob 区间，未找到返回 `null` |
| `findExtglobFrom` | `fn findExtglobFrom(str: string, start_pos: usize): ExtglobSpan?` | 从 `start_pos` 起查找下一个完整 extglob 区间，未找到返回 `null` |
| `findExtglobStart` | `fn findExtglobStart(str: string): i64` | 返回首个 extglob 的起始字节偏移，未找到返回 `-1` |
| `matchExtglobEnd` | `fn matchExtglobEnd(str: string, start: usize): usize` | 从前缀位置 `start` 匹配闭合 `)` 并返回右开结束索引，未闭合返回 `0` |
| `hasNestedExtglob` | `fn hasNestedExtglob(str: string): bool` | 判断是否包含嵌套 extglob（如 `+(a\|!(b\|c))`） |
| `isNegatedExtglob` | `fn isNegatedExtglob(str: string): bool` | 判断是否包含至少一个否定 extglob `!(...)`（含嵌套） |
| `countExtglobs` | `fn countExtglobs(str: string): usize` | 统计顶层完整 extglob 表达式个数 |
| `isExtglobPrefix` | `fn isExtglobPrefix(ch: u8): bool` | 判断字节是否为 `?`、`*`、`+`、`@`、`!` 之一 |
| `extglobKindFromByte` | `fn extglobKindFromByte(ch: u8): ExtglobKind?` | 将操作符字节映射为 `ExtglobKind`，非法返回 `null` |
| `extglobPrefixByte` | `fn extglobPrefixByte(kind: ExtglobKind): u8` | 将 `ExtglobKind` 还原为对应 ASCII 操作符字节 |

## 使用示例

### 1. 检测扩展通配符与嵌套/否定形态

```cp
import { isExtglob, isNegatedExtglob, hasNestedExtglob } from "is-extglob";

pub fn main() {
    const a = isExtglob("src/@(foo|bar).cp");        // true
    const b = isExtglob("src/\\@(foo|bar).cp");      // false（已转义）
    const c = isNegatedExtglob("!(node_modules)");   // true
    const d = hasNestedExtglob("+(a|!(b|c))");       // true
}
```

### 2. 遍历所有顶层 extglob 片段

```cp
import { findExtglobFrom, ExtglobSpan } from "is-extglob";
import { print } from "cap:io";

pub fn listExtglobs(pattern: string) {
    let pos: usize = 0;
    while (pos < pattern.len) {
        if (const span = findExtglobFrom(pattern, pos)) {
            print(pattern[span.start..span.end]);
            pos = span.end;
        } else {
            break;
        }
    }
}
```

### 3. 获取首个操作符类型

```cp
import { findExtglob, extglobPrefixByte } from "is-extglob";

pub fn firstOperator(pattern: string): u8 {
    if (const span = findExtglob(pattern)) {
        return extglobPrefixByte(span.kind);
    }
    return 0;
}
```

## 与源包差异与安全亮点

- **无正则回溯**：采用线性括号平衡状态机扫描，彻底规避复杂路径模式下的正则回溯开销。
- **结构化区间信息**：相比 npm 源包单一布尔值，额外提供 `ExtglobSpan`、`ExtglobKind`、`hasNestedExtglob`、`isNegatedExtglob`、`countExtglobs`，可直接供 glob 编译器使用。
- **零堆分配**：全部函数只读借用输入字符串，返回值全在栈上。

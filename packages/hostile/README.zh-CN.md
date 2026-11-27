[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# hostile

Zero-allocation POSIX/Windows /etc/hosts parser, case-insensitive forward/family/reverse lookup, and canonical formatter (hostile / Rust hostsfile parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:hostile` ([https://github.com/feross/hostile](https://github.com/feross/hostile))（原库名 `hostile` → CP 包名 `hostile`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `hostile` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add hostile
```

```cp
import {
    HostEntry,
    HostsTable,
    isBlank,
    parseHosts,
    lookupIp,
    lookupIpByFamily,
    reverseLookup,
    pushByte,
} from "hostile";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HostEntry` | `class` | 字段: `ip: string, hostname: string, is_ipv6: bool` · 方法: `empty` |
| `HostsTable` | `class` | 字段: `entries: [MAX_HOST_ENTRIES]HostEntry, count: usize` · 方法: `empty, entryAt` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isBlank` | `(c: u8): bool` | 执行 `isBlank` 条件判定，返回 `bool`。 |
| `parseHosts` | `(content: string): HostsTable` | 通过 `parseHosts` 解析输入数据，返回 `HostsTable`。 |
| `lookupIp` | `(table: HostsTable, hostname: string): string?` | 查询或读取目标数据，返回 `string?`。 |
| `lookupIpByFamily` | `(table: HostsTable, hostname: string, ipv6: bool): string?` | 查询或读取目标数据，返回 `string?`。 |
| `reverseLookup` | `(table: HostsTable, ip: string): string?` | 调用 `reverseLookup` 并返回 `string?`。 |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `formatHosts` | `(table: HostsTable, out: []u8): string` | 通过 `formatHosts` 编码或格式化输出，返回 `string`。 |

## 最小调用示例

```cp
import { parseHosts } from "hostile";

pub fn main() {
    let result = parseHosts("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

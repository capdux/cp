[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# macaddr

Zero-heap pure-CP IEEE 802 MAC-48/EUI-48 (MacAddr6) and EUI-64 (MacAddr8) hardware address parser, formatter, and RFC 4291 converter (crates.io macaddr equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:macaddr` ([https://github.com/svartalf/rust-macaddr](https://github.com/svartalf/rust-macaddr))

| 元信息 | 值 |
| --- | --- |
| 包名 | `macaddr` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add macaddr
```

```cp
import { MacAddr6, MacAddr8, macParseHexByte } from "macaddr";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `MacAddr6` | `class` | 字段: `bytes: [6]u8` · 方法: `new, fromArray, nil, broadcast, parse, isNil, +10 项` |
| `MacAddr8` | `class` | 字段: `bytes: [8]u8` · 方法: `fromArray, nil, broadcast, fromMac6Eui64, parse, isNil, +9 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `macParseHexByte` | `(hi_ch: u8, lo_ch: u8): u8?` | 调用 `macParseHexByte` 并返回 `u8?`。 |

## 最小调用示例

```cp
import { macParseHexByte } from "macaddr";

pub fn main() {
    let result = macParseHexByte(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

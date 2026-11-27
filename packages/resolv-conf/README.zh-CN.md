[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# resolv-conf

Zero-heap POSIX /etc/resolv.conf DNS resolver configuration parser

| 元信息 | 值 |
| --- | --- |
| 包名 | `resolv-conf` |
| 版本 | `0.7.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add resolv-conf
```

```cp
import { ResolvConfig, defaultResolvConfig, parseResolvConf } from "resolv-conf";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `ResolvConfig` | `class` | 字段: `nameservers: [8]string, nameserver_count: usize, search_domains: [8]string, search_count: usize, domain: string, ndots: u32, +6 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `defaultResolvConfig` | `(): ResolvConfig` | 调用 `defaultResolvConfig` 并返回 `ResolvConfig`。 |
| `parseResolvConf` | `(content: string): ResolvConfig` | 通过 `parseResolvConf` 解析输入数据，返回 `ResolvConfig`。 |

## 最小调用示例

```cp
import { parseResolvConf } from "resolv-conf";

pub fn main() {
    let result = parseResolvConf("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

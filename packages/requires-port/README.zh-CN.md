[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# requires-port

Zero-heap checker determining whether a port number is non-default for a given URL protocol

| 元信息 | 值 |
| --- | --- |
| 包名 | `requires-port` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add requires-port
```

```cp
import { rptNormalizeProtocol, defaultPortForProtocol, requiresPort } from "requires-port";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `rptNormalizeProtocol` | `(proto: string): string` | 调用 `rptNormalizeProtocol` 并返回 `string`。 |
| `defaultPortForProtocol` | `(protocol: string): u16` | 调用 `defaultPortForProtocol` 并返回 `u16`。 |
| `requiresPort` | `(port: u16, protocol: string): bool` | 调用 `requiresPort` 并返回 `bool`。 |

## 最小调用示例

```cp
import { rptNormalizeProtocol } from "requires-port";

pub fn main() {
    let result = rptNormalizeProtocol("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

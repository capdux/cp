[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# tower-http

Zero-heap CORS origin matcher, wildcard subdomain check, and preflight method/header validator for CP (compatible with npm cors & Rust tower-http::cors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:tower-http` ([https://github.com/tower-rs/tower-http](https://github.com/tower-rs/tower-http))（原库名 `tower-http` → CP 包名 `tower-http`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `tower-http` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add tower-http
```

```cp
import {
    findSchemeSep,
    isValidOrigin,
    matchOrigin,
    matchOriginList,
    isMethodAllowed,
    areHeadersAllowed,
    isSimpleMethod,
} from "tower-http";
```

## 核心 API

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `findSchemeSep` | `(s: string): i64` | 查询或读取目标数据，返回 `i64`。 |
| `isValidOrigin` | `(origin: string): bool` | 执行 `isValidOrigin` 条件判定，返回 `bool`。 |
| `matchOrigin` | `(origin: string, pattern: string, allow_credentials: bool): bool` | 调用 `matchOrigin` 并返回 `bool`。 |
| `matchOriginList` | `(origin: string, patterns: &[]string, allow_credentials: bool): bool` | 调用 `matchOriginList` 并返回 `bool`。 |
| `isMethodAllowed` | `(method: string, allowed_methods: string): bool` | 执行 `isMethodAllowed` 条件判定，返回 `bool`。 |
| `areHeadersAllowed` | `(req_headers: string, allowed_headers: string): bool` | 调用 `areHeadersAllowed` 并返回 `bool`。 |
| `isSimpleMethod` | `(method: string): bool` | 执行 `isSimpleMethod` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isValidOrigin } from "tower-http";

pub fn main() {
    let result = isValidOrigin("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

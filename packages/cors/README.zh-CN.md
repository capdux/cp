[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# cors

Zero-heap Fetch/W3C CORS origin evaluator and preflight/actual response header builder (clean-room port of npm cors)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:cors` ([https://github.com/expressjs/cors](https://github.com/expressjs/cors))（原库名 `cors` → CP 包名 `cors`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `cors` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add cors
```

```cp
import {
    CorsOptions,
    CorsDecision,
    corsPush,
    corsPushU32,
    corsIsWs,
    corsTrim,
    isOriginAllowed,
    isMethodAllowed,
} from "cors";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `CorsOptions` | `class` | 字段: `allow_any_origin: bool, reflect_origin: bool, allowed_origins: [16]string, allowed_origin_count: usize, allowed_methods: string, allowed_headers: string, +4 项` · 方法: `permissive, restrictive, addOrigin` |
| `CorsDecision` | `class` | 字段: `allowed: bool, is_preflight: bool, allow_origin: string, vary_origin: bool, allow_credentials: bool, allow_methods: string, +4 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `corsPush` | `(out: []u8, pos: usize, s: string): usize` | 调用 `corsPush` 并返回 `usize`。 |
| `corsPushU32` | `(out: []u8, pos: usize, v: u32): usize` | 调用 `corsPushU32` 并返回 `usize`。 |
| `corsIsWs` | `(b: u8): bool` | 调用 `corsIsWs` 并返回 `bool`。 |
| `corsTrim` | `(s: string): string` | 调用 `corsTrim` 并返回 `string`。 |
| `isOriginAllowed` | `(opts: CorsOptions, request_origin: string): bool` | 执行 `isOriginAllowed` 条件判定，返回 `bool`。 |
| `isMethodAllowed` | `(allowed_methods: string, method: string): bool` | 执行 `isMethodAllowed` 条件判定，返回 `bool`。 |
| `evaluateCors` | `(opts: CorsOptions, method: string, origin: string, ac_req_method: string, ac_req_headers: string): CorsDecision` | 调用 `evaluateCors` 并返回 `CorsDecision`。 |
| `formatCorsHeaders` | `(d: CorsDecision, out: []u8): usize` | 通过 `formatCorsHeaders` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isOriginAllowed } from "cors";

pub fn main() {
    let result = isOriginAllowed(/* opts: CorsOptions */, "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

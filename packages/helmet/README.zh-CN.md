[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# helmet

Zero-heap HTTP security response headers policy generator (CSP, HSTS, COOP, CORP, X-Frame-Options, Referrer-Policy) with CRLF-injection defense in pure CP (clean-room MIT counterpart to npm helmet v8).

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:helmet` ([https://github.com/helmetjs/helmet](https://github.com/helmetjs/helmet))（原库名 `helmet` → CP 包名 `helmet`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `helmet` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add helmet
```

```cp
import {
    HelmetOptions,
    hlmWriteByte,
    hlmWriteStr,
    hlmWriteU64,
    hlmWriteHeaderLine,
    hlmAppendCspDirective,
    isSafeHeaderValue,
    formatHstsHeader,
} from "helmet";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `HelmetOptions` | `class` | 字段: `csp_enabled: bool, csp_value: string, hsts_enabled: bool, hsts_max_age: u64, hsts_include_subdomains: bool, hsts_preload: bool, +8 项` · 方法: `defaultStrict, minimal` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `hlmWriteByte` | `(out: []u8, pos: &mut usize, b: u8)` | 就地执行 `hlmWriteByte` 操作。 |
| `hlmWriteStr` | `(out: []u8, pos: &mut usize, s: string)` | 就地执行 `hlmWriteStr` 操作。 |
| `hlmWriteU64` | `(out: []u8, pos: &mut usize, val: u64)` | 就地执行 `hlmWriteU64` 操作。 |
| `hlmWriteHeaderLine` | `(out: []u8, pos: &mut usize, name: string, val: string)` | 就地执行 `hlmWriteHeaderLine` 操作。 |
| `hlmAppendCspDirective` | `(out: []u8, pos: &mut usize, has_any: &mut bool, dir_name: string, val: string)` | 就地执行 `hlmAppendCspDirective` 操作。 |
| `isSafeHeaderValue` | `(val: string): bool` | 执行 `isSafeHeaderValue` 条件判定，返回 `bool`。 |
| `formatHstsHeader` | `(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize` | 通过 `formatHstsHeader` 编码或格式化输出，返回 `usize`。 |
| `formatCspHeader` | `(default_src: string, script_src: string, style_src: string, img_src: string, connect_src: string, out: []u8): usize` | 通过 `formatCspHeader` 编码或格式化输出，返回 `usize`。 |
| `formatHelmetHeaders` | `(opts: HelmetOptions, out: []u8): usize` | 通过 `formatHelmetHeaders` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isSafeHeaderValue } from "helmet";

pub fn main() {
    let result = isSafeHeaderValue("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

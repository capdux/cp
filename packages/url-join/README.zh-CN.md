[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# url-join

Zero-allocation smart URL joiner, slash/dot-segment path normalizer, query merger, and trailing-slash utilities (url-join / ufo parity) for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:url-join` ([https://github.com/jfromaniello/url-join](https://github.com/jfromaniello/url-join))

| 元信息 | 值 |
| --- | --- |
| 包名 | `url-join` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add url-join
```

```cp
import {
    PartSplit,
    pushByte,
    isSchemeChar,
    schemePrefixLen,
    splitPart,
    trimLeadingSlashes,
    trimTrailingSlashes,
    urlJoin,
} from "url-join";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PartSplit` | `class` | 字段: `path: string, query: string, hash: string` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `pushByte` | `(out: []u8, pos: usize, b: u8): usize` | 写入或更新状态数据。 |
| `isSchemeChar` | `(c: u8): bool` | 执行 `isSchemeChar` 条件判定，返回 `bool`。 |
| `schemePrefixLen` | `(s: string): usize` | 调用 `schemePrefixLen` 并返回 `usize`。 |
| `splitPart` | `(p: string): PartSplit` | 调用 `splitPart` 并返回 `PartSplit`。 |
| `trimLeadingSlashes` | `(s: string): string` | 调用 `trimLeadingSlashes` 并返回 `string`。 |
| `trimTrailingSlashes` | `(s: string): string` | 调用 `trimTrailingSlashes` 并返回 `string`。 |
| `urlJoin` | `(parts: []string, out: []u8): string` | 调用 `urlJoin` 并返回 `string`。 |
| `normalizeUrlPath` | `(path: string, out: []u8): string` | 调用 `normalizeUrlPath` 并返回 `string`。 |
| `withQuery` | `(base_url: string, key: string, value: string, out: []u8): string` | 调用 `withQuery` 并返回 `string`。 |
| `findQueryOrHash` | `(url: string): usize` | 查询或读取目标数据，返回 `usize`。 |
| `withTrailingSlash` | `(url: string, out: []u8): string` | 调用 `withTrailingSlash` 并返回 `string`。 |
| `withoutTrailingSlash` | `(url: string, out: []u8): string` | 调用 `withoutTrailingSlash` 并返回 `string`。 |

## 最小调用示例

```cp
import { schemePrefixLen } from "url-join";

pub fn main() {
    let result = schemePrefixLen("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# packageurl-js

Zero-allocation Package URL (PURL) parser, validator, qualifier lookup, and canonical serializer (SBOM standard) in pure CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:packageurl-js` ([https://github.com/package-url/packageurl-js](https://github.com/package-url/packageurl-js)) / `crates.io:packageurl` ([https://github.com/althonos/packageurl.rs](https://github.com/althonos/packageurl.rs))（原库名 `packageurl-js` → CP 包名 `packageurl-js`）

| 元信息 | 值 |
| --- | --- |
| 包名 | `packageurl-js` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add packageurl-js
```

```cp
import {
    Purl,
    invalidPurl,
    purlIsSpace,
    isValidType,
    trimSlashes,
    parsePurl,
    getQualifier,
    pushSlice,
} from "packageurl-js";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `Purl` | `class` | 字段: `pkg_type: string, namespace: string, name: string, version: string, qualifiers: string, subpath: string, +1 项` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `invalidPurl` | `(): Purl` | 调用 `invalidPurl` 并返回 `Purl`。 |
| `purlIsSpace` | `(b: u8): bool` | 调用 `purlIsSpace` 并返回 `bool`。 |
| `isValidType` | `(pkg_type: string): bool` | 执行 `isValidType` 条件判定，返回 `bool`。 |
| `trimSlashes` | `(s: string): string` | 调用 `trimSlashes` 并返回 `string`。 |
| `parsePurl` | `(input: string): Purl` | 通过 `parsePurl` 解析输入数据，返回 `Purl`。 |
| `getQualifier` | `(purl: Purl, key: string): string` | 查询或读取目标数据，返回 `string`。 |
| `pushSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `pushLowerSlice` | `(dst: []u8, pos: usize, src: string): i64` | 写入或更新状态数据。 |
| `formatPurl` | `(p: Purl, out: []u8): usize` | 通过 `formatPurl` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidType } from "packageurl-js";

pub fn main() {
    let result = isValidType("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

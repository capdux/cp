[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# npm-package-arg

Zero-heap npm package name validator and package specifier (@scope/name@version) parser and formatter.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **npm** [`npm-package-arg`](https://github.com/npm/npm-package-arg)（`ISC`）（原库名 `npm-package-arg`，CP 包名 `npm-package-arg`）。

| 元信息 | 值 |
| --- | --- |
| 包名 | `npm-package-arg` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add npm-package-arg
```

```cp
import {
    PackageSpec,
    isValidSegment,
    isValidPackageName,
    parsePackageSpec,
    appendSlice,
    formatPackageSpec,
} from "npm-package-arg";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PackageSpec` | `class` | 字段: `scope: string, name: string, full_name: string, version_spec: string, is_scoped: bool, valid: bool` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `isValidSegment` | `(seg: string): bool` | 执行 `isValidSegment` 条件判定，返回 `bool`。 |
| `isValidPackageName` | `(name: string): bool` | 执行 `isValidPackageName` 条件判定，返回 `bool`。 |
| `parsePackageSpec` | `(input: string): PackageSpec` | 通过 `parsePackageSpec` 解析输入数据，返回 `PackageSpec`。 |
| `appendSlice` | `(out: []u8, pos: usize, s: string): i64` | 调用 `appendSlice` 并返回 `i64`。 |
| `formatPackageSpec` | `(spec: PackageSpec, out: []u8): usize` | 通过 `formatPackageSpec` 编码或格式化输出，返回 `usize`。 |

## 最小调用示例

```cp
import { isValidSegment } from "npm-package-arg";

pub fn main() {
    let result = isValidSegment("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

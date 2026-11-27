[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# normalize-package-data

Zero-allocation package metadata normalizer and validator for CP, reusing validate-npm-package-name and validate-npm-package-license.

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:normalize-package-data` ([https://github.com/npm/normalize-package-data](https://github.com/npm/normalize-package-data))

| 元信息 | 值 |
| --- | --- |
| 包名 | `normalize-package-data` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add normalize-package-data
```

```cp
import {
    PackageMetadata,
    NormalizedPackageResult,
    npdIsOwnerRepoShorthand,
    npdWriteHostedUrl,
    expandRepoShorthand,
    normalizeEntryPath,
    cleanVersionString,
    normalizePackageData,
} from "normalize-package-data";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PackageMetadata` | `class` | 字段: `name: string, version: string, license: string, repository: string, main_entry: string, private_pkg: bool` |
| `NormalizedPackageResult` | `class` | 字段: `valid: bool, name_valid: bool, license_valid: bool, has_warnings: bool, warning_count: usize` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `npdIsOwnerRepoShorthand` | `(s: string): bool` | 调用 `npdIsOwnerRepoShorthand` 并返回 `bool`。 |
| `npdWriteHostedUrl` | `(base_url: string, slug: string, out: []u8): usize` | 调用 `npdWriteHostedUrl` 并返回 `usize`。 |
| `expandRepoShorthand` | `(repo: string, out: []u8): usize` | 调用 `expandRepoShorthand` 并返回 `usize`。 |
| `normalizeEntryPath` | `(entry: string): string` | 调用 `normalizeEntryPath` 并返回 `string`。 |
| `cleanVersionString` | `(ver: string): string` | 调用 `cleanVersionString` 并返回 `string`。 |
| `normalizePackageData` | `(meta: PackageMetadata): NormalizedPackageResult` | 调用 `normalizePackageData` 并返回 `NormalizedPackageResult`。 |

## 最小调用示例

```cp
import { npdIsOwnerRepoShorthand } from "normalize-package-data";

pub fn main() {
    let result = npdIsOwnerRepoShorthand("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

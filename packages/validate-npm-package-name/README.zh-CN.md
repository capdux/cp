[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# validate-npm-package-name

Zero-heap npm/cap package name validator with scoped @org/pkg rules, reserved names, and legacy vs new validation (npm validate-npm-package-name equivalent)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:validate-npm-package-name` ([https://github.com/npm/validate-npm-package-name](https://github.com/npm/validate-npm-package-name))

| 元信息 | 值 |
| --- | --- |
| 包名 | `validate-npm-package-name` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add validate-npm-package-name
```

```cp
import {
    PackageNameValidation,
    vnpStrEq,
    vnpEqIgnoreCase,
    vnpIsUrlSafeChar,
    vnpIsSpecialWarnChar,
    isBlacklistedName,
    isCoreModuleName,
    vnpCheckSegment,
} from "validate-npm-package-name";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `PackageNameValidation` | `class` | 字段: `valid_for_new: bool, valid_for_old: bool, error_code: u8, warning_code: u8` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `vnpStrEq` | `(a: string, b: string): bool` | 调用 `vnpStrEq` 并返回 `bool`。 |
| `vnpEqIgnoreCase` | `(a: string, b: string): bool` | 调用 `vnpEqIgnoreCase` 并返回 `bool`。 |
| `vnpIsUrlSafeChar` | `(c: u8): bool` | 调用 `vnpIsUrlSafeChar` 并返回 `bool`。 |
| `vnpIsSpecialWarnChar` | `(c: u8): bool` | 调用 `vnpIsSpecialWarnChar` 并返回 `bool`。 |
| `isBlacklistedName` | `(name: string): bool` | 执行 `isBlacklistedName` 条件判定，返回 `bool`。 |
| `isCoreModuleName` | `(name: string): bool` | 执行 `isCoreModuleName` 条件判定，返回 `bool`。 |
| `vnpCheckSegment` | `(seg: string): u8` | 调用 `vnpCheckSegment` 并返回 `u8`。 |
| `validatePackageName` | `(name: string): PackageNameValidation` | 校验输入合法性，返回 `PackageNameValidation`。 |
| `isValidNewPackageName` | `(name: string): bool` | 执行 `isValidNewPackageName` 条件判定，返回 `bool`。 |

## 最小调用示例

```cp
import { isBlacklistedName } from "validate-npm-package-name";

pub fn main() {
    let result = isBlacklistedName("example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# `validate-npm-package-license`

> **源包溯源**：`npm:validate-npm-package-license` ([kemitchell/validate-npm-package-license.js](https://github.com/kemitchell/validate-npm-package-license.js)) — 协议：MIT（上游：Apache-2.0）

纯 CP 实现的零堆分配 npm / cap 包许可证字段校验器，完整支持 SPDX 复合表达式、`UNLICENSED`、`SEE LICENSE IN <file>`、历史遗留值，以及 OSI 认证与 FSF 自由许可证 AST 求值。

## 零成本迁移对照（TS → CP）

直接沿用原包名 `validate-npm-package-license`，无需改名：

```typescript
// TypeScript / Node.js (npm: validate-npm-package-license)
import validate from "validate-npm-package-license";
const res = validate("(MIT OR Apache-2.0)");
// { validForNewPackages: true, validForOldPackages: true, spdx: true }
```

```cp
// CP（零堆分配校验 + OSI/FSF 合规分析）
import { validateLicense, inspectLicense } from "validate-npm-package-license";

let res = validateLicense("(MIT OR Apache-2.0)");
// res.valid_for_new_packages == true, res.spdx == true

let info = inspectLicense("(MIT OR Apache-2.0)");
// info.osi_approved == true, info.fsf_libre == true, info.has_compound_expr == true
```

## 安装配置

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "validate-npm-package-license": "1.0.0"
  }
}
```

## 核心 API 速览

| 符号 | 签名 | 说明 |
| :--- | :--- | :--- |
| `LicenseValidation` | `class { valid_for_new_packages: bool, valid_for_old_packages: bool, spdx: bool, unlicensed: bool, in_file: string }` | 标准 npm 许可证校验结果 |
| `LicenseDetails` | `class { valid_for_new_packages: bool, valid_for_old_packages: bool, spdx: bool, unlicensed: bool, in_file: string, osi_approved: bool, fsf_libre: bool, has_compound_expr: bool, has_exception: bool }` | 扩展详情（含 OSI/FSF 状态、复合表达式与例外条款标记） |
| `validateLicense` | `(license: string): LicenseValidation` | 校验 SPDX、`UNLICENSED`、`SEE LICENSE IN <file>` 或历史许可证 |
| `inspectLicense` | `(license: string): LicenseDetails` | 一站式校验并解析 OSI、FSF、复合表达式与例外条款 |
| `isValidNewLicense` | `(license: string): bool` | 是否允许用于新发布的包 |
| `isValidOldLicense` | `(license: string): bool` | 是否兼容历史已发布的旧包 |
| `isOsiApprovedLicense` | `(license_id: string): bool` | 判断单个 SPDX ID 是否通过 OSI 认证 |
| `isFsfLibreLicense` | `(license_id: string): bool` | 判断单个 SPDX ID 是否为 FSF 自由许可证 |
| `isOsiApprovedExpression` | `(expr: string): bool` | 对完整 SPDX `AND`/`OR` 表达式进行 OSI 合规求值 |
| `isFsfLibreExpression` | `(expr: string): bool` | 对完整 SPDX `AND`/`OR` 表达式进行 FSF 自由度求值 |
| `vnlContainsWhitespace` / `vnlIsLegacyLicense` / `vnlEvalOsiAst` / `vnlEvalFsfAst` | 底层辅助函数 | 历史许可证判定与 AST 递归求值辅助 |

## 实战示例

### 1. 校验包许可证字段

```cp
import { validateLicense, isValidNewLicense, isValidOldLicense } from "validate-npm-package-license";

let r1 = validateLicense("MIT");
// r1.valid_for_new_packages == true, r1.spdx == true

let r2 = validateLicense("UNLICENSED");
// r2.valid_for_new_packages == true, r2.unlicensed == true

let r3 = validateLicense("SEE LICENSE IN LICENSE.txt");
// r3.valid_for_new_packages == true, r3.in_file == "LICENSE.txt"

let legacy_new = isValidNewLicense("BSD"); // false（新包禁止）
let legacy_old = isValidOldLicense("BSD"); // true（兼容旧包）
```

### 2. OSI 认证与 FSF 自由许可证表达式求值

```cp
import { inspectLicense, isOsiApprovedExpression, isFsfLibreExpression } from "validate-npm-package-license";

let osi = isOsiApprovedExpression("MIT AND Apache-2.0"); // true
let fsf = isFsfLibreExpression("MIT OR GPL-3.0-only");   // true

let details = inspectLicense("Apache-2.0 WITH LLVM-exception");
// details.spdx == true, details.has_exception == true
```

## 注意事项与边界说明

- **`SEE LICENSE IN <file>`**：文件名不能为空且不能包含空白字符；`in_file` 直接零拷贝切片自输入字符串。
- **布尔 AST 规则**：`A AND B` 要求两侧均满足 OSI/FSF；`A OR B` 只需任一侧满足即可。
- **零堆分配**：基于 `spdx-expression-parse` 与 `spdx` 栈上固定 AST 实现，无任何动态内存分配。

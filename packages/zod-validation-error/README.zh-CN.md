[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# zod-validation-error

面向 CP 的零堆内存可读校验错误聚合与格式化库（`fromZodError` / `fromZodErrorInto` / `fromError` / `createMessage` / `formatIssuePath` / `isValidationError`）。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/zod-validation-error](https://www.npmjs.com/package/zod-validation-error) |
| **上游原包名** | `zod-validation-error` v3.4.0 |
| **上游仓库** | https://github.com/causaly/zod-validation-error |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import { fromZodError, isValidationError } from "zod-validation-error";
const err = fromZodError(zodError, { prefix: "Validation error", issueSeparator: "; " });
```

```cp
// CP —— 原包名直接导入，零堆可读校验错误聚合
import {
    ValidationIssue, ValidationError,
    defaultMessageBuilderOptions, fromZodErrorInto,
    fromZodError, fromError, isValidationError,
} from "zod-validation-error";
```

## 安装

```bash
cap add zod-validation-error
```

## 实战示例

### 1. 零堆构造 `ValidationError`（`fromZodErrorInto` & `createMessage`）

```cp
import {
    ValidationIssue, ValidationError,
    defaultMessageBuilderOptions, fromZodErrorInto,
    createMessage, isValidationError, isValidationErrorLike,
} from "zod-validation-error";

fn demoValidationError() {
    let issues: [2]ValidationIssue = [
        ValidationIssue { path: "user.email", message: "Invalid email", code: "invalid_string" },
        ValidationIssue { path: "items[0].qty", message: "Expected >= 1", code: "too_small" },
    ];

    mut opts = defaultMessageBuilderOptions();
    opts.prefix = "Validation error";
    opts.prefix_separator = ": ";
    opts.issue_separator = "; ";
    opts.include_path = true;
    opts.include_code = false;

    // 零堆写入栈缓冲区并构造 ValidationError 包装对象
    mut msg_buf: [256]u8 = [0; 256];
    let err: ValidationError = fromZodErrorInto(&issues, opts, &mut msg_buf);

    let text = err.toString();             // "Validation error: Invalid email at \"user.email\"; Expected >= 1 at \"items[0].qty\""
    let total = err.issueCount();          // 2
    let truncated = err.isTruncated();     // false
    let ok_type = err.isValidationError(); // true
    let by_name = isValidationError(err.name); // true
    let like = isValidationErrorLike(err.name, err.detailsCount() > 0); // true

    // 或直接通过 createMessage 格式化输出字节长度
    mut out2: [256]u8 = [0; 256];
    let written = createMessage(&issues, opts, &mut out2);
}
```

### 2. 路径段格式化与普通错误包装

```cp
import {
    ValidationIssue, defaultFormatErrorOptions, defaultMessageBuilderOptions,
    formatIssuePath, formatValidationIssue, formatValidationIssues,
    fromErrorMessage, fromZodError,
} from "zod-validation-error";

fn demoFormatting() {
    // JS 风格点/方括号路径：["users", "0", "first-name"] -> users[0]["first-name"]
    let segs: [3]string = ["users", "0", "first-name"];
    mut path_buf: [128]u8 = [0; 128];
    let plen = formatIssuePath(&segs, &mut path_buf);

    // 格式化单条或批量校验错误
    let issue = ValidationIssue { path: "port", message: "Must be integer", code: "invalid_type" };
    mut line_buf: [128]u8 = [0; 128];
    let l1 = formatValidationIssue(issue, true, true, &mut line_buf);

    let list: [1]ValidationIssue = [issue];
    let fopts = defaultFormatErrorOptions();
    let l2 = formatValidationIssues(&list, fopts, &mut line_buf);

    // 包装普通错误信息字符串
    let wrapped = fromErrorMessage("Unexpected token", defaultMessageBuilderOptions());
    let msg = wrapped.message();
}
```

## 性能与安全亮点

- **零堆分配**：`fromZodErrorInto`、`createMessage`、`formatValidationIssues` 与 `formatIssuePath` 全部直接写入调用方栈切片 `[]u8`，零动态内存分配。
- **缓冲区防溢出**：写入辅助函数严格检查 `out.len`，永不越界，并返回精确写入字节数。
- **可控截断**：支持通过 `max_issues_in_message`/`max_issues` 限制最大展示条数，并提供 `isTruncated()`、`detailsCount()` 与 `issueCount()` 状态查询。
- **智能路径格式化**：自动区分数组索引（`[0]`）、合法标识符（`.name`）与含特殊字符键名（`["a-b"]`）。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。

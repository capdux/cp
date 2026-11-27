[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zod-validation-error

适用于 CP 语言的人类可读校验错误聚合与格式化库，对齐 npm [`zod-validation-error`](https://github.com/causaly/zod-validation-error) v3.4.0（MIT 协议，CP v3.4.0）。

可将结构化校验问题列表格式化为清晰易读的错误摘要，支持 JS 风格路径格式化（`users[0].email`、`config["api-key"]`）、自定义前缀与分隔符、问题数量截断（`max_issues_in_message`）、错误码展示，同时提供零堆分配路径（`fromZodErrorInto` / `createMessage`）与便捷构造路径（`fromZodError` / `fromError`）。

## 安装与导入

```bash
cap add zod-validation-error
```

```cp
import {
    ValidationIssue,
    FormatErrorOptions,
    MessageBuilderOptions,
    ValidationError,
    defaultMessageBuilderOptions,
    createMessage,
    fromZodErrorInto,
    fromZodError,
    formatIssuePath,
} from "zod-validation-error";
```

## 核心 API 参考

### 常量与数据结构

| 符号 | 字段 / 方法 | 说明 |
| :--- | :--- | :--- |
| `VALIDATION_ERROR_NAME` | `"ZodValidationError"` | 标准 `ValidationError.name` 标识。 |
| `ValidationIssue` | `path: string`, `message: string`, `code: string` | 单条校验问题描述符。 |
| `FormatErrorOptions` | `prefix`, `prefix_sep`, `issue_sep`, `max_issues`, `include_path` | 供 `formatValidationIssues` 使用的精简格式选项。 |
| `MessageBuilderOptions` | `prefix`, `prefix_separator`, `issue_separator`, `union_separator`, `max_issues_in_message`, `include_path`, `include_code` | 对齐 npm `zod-validation-error` 的完整消息构建选项。 |
| `ValidationError` | `name`, `message_text`, `details_count`, `total_issues`, `truncated` | 结构化错误对象，提供 `message()`、`toString()`、`copyMessage(out: []u8): usize`、`issueCount()`、`detailsCount()`、`isTruncated()`、`isValidationError()`。 |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `defaultFormatErrorOptions` | `(): FormatErrorOptions` | 默认精简选项（`prefix: "Validation error"`、`prefix_sep: ": "`、`issue_sep: "; "`）。 |
| `defaultMessageBuilderOptions` | `(): MessageBuilderOptions` | 对齐 npm 原版的完整默认消息选项。 |
| `formatIssuePath` | `(segments: &[]string, out: []u8): usize` | 将路径分段格式化为点号、数字索引 `[0]` 或引号 `["a-b"]` 形式写入 `out`。 |
| `formatValidationIssue` | `(issue: ValidationIssue, include_path: bool, include_code: bool, out: []u8): usize` | 格式化单条校验问题写入 `out`，返回写入字节数。 |
| `formatValidationIssues` | `(issues: &[]ValidationIssue, opts: FormatErrorOptions, out: []u8): usize` | 零堆分配批量格式化问题切片。 |
| `createMessage` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): usize` | 零堆分配完整消息构建器，写入 `out` 并返回字节数。 |
| `fromZodErrorInto` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions, out: []u8): ValidationError` | 零堆分配构造器，返回的 `ValidationError.message_text` 直接引用 `out[0..n]`。 |
| `fromZodError` / `fromError` | `(issues: &[]ValidationIssue, opts: MessageBuilderOptions): ValidationError` | 便捷构造器，返回自持消息的 `ValidationError`（单条消息上限 512 字节）。 |
| `fromErrorMessage` | `(msg: string, opts: MessageBuilderOptions): ValidationError` | 将单条纯文本错误消息包装为 `ValidationError`。 |
| `isValidationError` | `(error_name: string): bool` | 检查 `error_name == "ZodValidationError"`。 |
| `isValidationErrorLike` | `(error_name: string, has_details: bool): bool` | 鸭子类型校验错误对象判定。 |

## 代码示例

### 1. 零堆分配构造（`fromZodErrorInto`）

```cp
import {
    ValidationIssue,
    defaultMessageBuilderOptions,
    fromZodErrorInto,
} from "zod-validation-error";

fn formatUserErrors() {
    let issues = [
        ValidationIssue { path: "email", message: "Invalid email", code: "invalid_string" },
        ValidationIssue { path: "age", message: "Must be >= 18", code: "too_small" },
    ];
    let opts = defaultMessageBuilderOptions();
    let buf: [256]u8 = undefined;
    let err = fromZodErrorInto(&issues, opts, buf[0..256]);
    // err.message() -> "Validation error: Invalid email at \"email\"; Must be >= 18 at \"age\""
    // err.issueCount() == 2, err.isTruncated() == false
}
```

### 2. JS 风格问题路径格式化

```cp
import { formatIssuePath } from "zod-validation-error";

fn buildPath(): usize {
    let segs = ["users", "0", "api-key"];
    let out: [64]u8 = undefined;
    return formatIssuePath(&segs, out[0..64]);
    // 写入: users[0]["api-key"]
}
```

### 3. 限制展示条目数与附带错误码

```cp
import {
    ValidationIssue,
    defaultMessageBuilderOptions,
    createMessage,
} from "zod-validation-error";

fn truncatedSummary(): usize {
    let issues = [
        ValidationIssue { path: "a", message: "Required", code: "invalid_type" },
        ValidationIssue { path: "b", message: "Required", code: "invalid_type" },
    ];
    let mut opts = defaultMessageBuilderOptions();
    opts.max_issues_in_message = 1;
    opts.include_code = true;
    let out: [256]u8 = undefined;
    return createMessage(&issues, opts, out[0..256]);
}
```

## 与源包差异与零拷贝安全亮点

- **零堆分配快路径**：`createMessage`、`formatValidationIssues`、`formatIssuePath` 与 `fromZodErrorInto` 均直接写入调用方缓冲区，适合高频服务端请求校验热路径。
- **与 Schema 库解耦**：通过通用 `&[]ValidationIssue` 切片接收错误列表，不强绑定单一校验框架。
- **边界安全截断**：同时遵守 `max_issues_in_message` 与输出切片容量限制，绝不越界写入。

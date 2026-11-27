[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# jest-docblock

纯 CP 实现的零堆文件头 Docblock（`/** ... */`）提取、剥离、指令（Pragma）解析与格式化库。

- **源包对齐**：参考 npm [`jest-docblock`](https://github.com/jestjs/jest) `29.7.0`（`MIT`）。
- **包版本**：`29.7.0`

## 安装与导入

```bash
cap add jest-docblock
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "jest-docblock": "^29.7.0"
  }
}
```

```typescript
import {
    DocPragma, ParsedDocblock,
    extract, extractDocblock, strip, stripSlice,
    parse, parseDocblock, parseWithComments,
    print, printDocblock, printWithComments,
    getPragma, hasPragma
} from "jest-docblock";
```

## 核心 API 参考

### 数据结构

| 类型 | 字段 | 说明 |
| :--- | :--- | :--- |
| `DocPragma` | `key: string`、`value: string` | 单条 `@key value` 指令（零拷贝借用自源字符串） |
| `ParsedDocblock` | `comments_len: usize`、`pragma_count: usize` | `parseWithComments` 返回的注释字节数与指令条数 |

### 核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `extract` / `extractDocblock` | `extract(code: string): string` | 提取源码顶部 `/** ... */` 注释块（零拷贝切片） |
| `strip` | `strip(code: string, out: []u8): usize` | 去除顶部 docblock 并将剩余源码写入 `out`，返回写入字节数 |
| `stripSlice` | `stripSlice(code: string): string` | 零拷贝返回剥离顶部 docblock 后的源码切片 |
| `parse` / `parseDocblock` | `parse(docblock: string, out: []DocPragma): usize` | 解析所有 `@key value` 指令写入 `out`，返回解析条数 |
| `parseWithComments` | `parseWithComments(docblock: string, comments_out: []u8, pragmas_out: []DocPragma): ParsedDocblock` | 同时分离正文注释（写入 `comments_out`）与指令（写入 `pragmas_out`） |
| `print` / `printDocblock` | `print(pragmas: &[]DocPragma, out: []u8): usize` | 将指令切片格式化为 `/** ... */` 写入 `out` |
| `printWithComments` | `printWithComments(comments: string, pragmas: &[]DocPragma, out: []u8): usize` | 将正文注释与指令一起格式化为标准 docblock |
| `getPragma` | `getPragma(pragmas: &[]DocPragma, key: string): string?` | 按 `key` 查询首条匹配指令值，不存在返回 `null` |
| `hasPragma` | `hasPragma(pragmas: &[]DocPragma, key: string): bool` | 判断指令切片中是否包含 `key` |

## 使用示例

### 1. 提取并读取 `@jest-environment` 指令

```typescript
import { extract, parse, getPragma, hasPragma, DocPragma } from "jest-docblock";

fn detectEnv(src: string): string {
    let doc = extract(src);
    let mut pragmas: [16]DocPragma = undefined;
    let count = parse(doc, &mut pragmas);
    let slice = pragmas[0..count];

    if (hasPragma(slice, "jest-environment")) {
        return getPragma(slice, "jest-environment") orelse "node";
    }
    return "node";
}
```

### 2. 带注释解析并重新格式化输出

```typescript
import { parseWithComments, printWithComments, DocPragma } from "jest-docblock";

fn reformatDocblock(doc: string, out: []u8): usize {
    let mut comments_buf: [256]u8 = undefined;
    let mut pragmas: [16]DocPragma = undefined;
    let parsed = parseWithComments(doc, &mut comments_buf, &mut pragmas);

    let comments = comments_buf[0..parsed.comments_len] as string;
    return printWithComments(comments, pragmas[0..parsed.pragma_count], out);
}
```

## 与源包差异与 CP 安全亮点

- **零拷贝提取与剥离**：`extract` 与 `stripSlice` 直接返回原始源码的子切片，无任何堆分配。
- **调用方缓冲写入**：`parse`、`parseWithComments`、`strip`、`print`、`printWithComments` 均写入调用方传入的可写切片并返回实际写入数量。
- **类型安全可选返回**：`getPragma` 返回 `string?`，编译期强制处理空值分支。

[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# media-typer

纯 CP 实现的零堆 RFC 6838 HTTP 媒体类型解析、校验、分类与规范小写格式化库。

- **源包对齐**：参考 npm [`media-typer`](https://github.com/jshttp/media-typer)（`MIT`）净室重构。
- **包版本**：`1.0.0`（依赖 `unicase`）。

## 安装与导入

```bash
cap add media-typer
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "media-typer": "^1.0.0"
  }
}
```

```typescript
import {
    MediaType, parse, parseMediaType, format, formatMediaType,
    testMediaType, testType, essence, mediaTypeEq, hasSuffix,
    isValidType, isValidSubtype, isValidSuffix,
    isStandardType, isVendorSubtype, isPersonalSubtype, isUnregisteredSubtype
} from "media-typer";
```

## 核心 API 参考

### `class MediaType`

字段：`valid: bool`、`main_type: string`、`subtype: string`、`suffix: string`。

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `validate` | `MediaType.validate(raw: string): bool` | 静态 RFC 6838 语法校验 |
| `isValid` | `isValid(): bool` | 判断解析结果是否有效 |
| `hasSuffix` | `hasSuffix(): bool` | 判断是否带有结构化后缀（`+suffix`） |
| `formatTo` | `formatTo(out: []u8): usize` | 以规范小写 `type/subtype[+suffix]` 写入 `out` |

### 核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parse` / `parseMediaType` | `parse(raw: string): MediaType` | 零拷贝解析 `type/subtype[+suffix]`（非法时 `valid = false`） |
| `testMediaType` / `testType` | `testMediaType(raw: string): bool` | 校验原始字符串是否符合 RFC 6838 媒体类型格式 |
| `format` | `format(mt: MediaType, out: []u8): usize` | 将 `MediaType` 规范化为小写写入 `out`；无效返回 `0` |
| `formatMediaType` | `formatMediaType(main_type: string, subtype: string, suffix: string, out: []u8): usize` | 校验各段并格式化为小写写入 `out` |
| `essence` | `essence(raw: string, out: []u8): usize` | 解析 `raw` 并输出规范小写媒体类型本质 |
| `mediaTypeEq` | `mediaTypeEq(a: string, b: string): bool` | 忽略 ASCII 大小写的 RFC 6838 等价比较 |
| `hasSuffix` | `hasSuffix(raw: string, suffix: string): bool` | 忽略大小写判断 `raw` 是否具备指定后缀（如 `"json"`、`"xml"`） |
| `isValidType` / `isValidSubtype` / `isValidSuffix` | `isValidType(s: string): bool` | 按段校验主类型 / 子类型 / 后缀合法性 |
| `isStandardType` | `isStandardType(t: string): bool` | 判断是否为标准顶级类型（`text`、`application`、`image` 等） |
| `isVendorSubtype` / `isPersonalSubtype` / `isUnregisteredSubtype` | `isVendorSubtype(subtype: string): bool` | 检测 `vnd.`、`prs.`、`x.` 注册树前缀 |

## 使用示例

### 1. 解析媒体类型并检测厂商树与后缀

```typescript
import { parse, isVendorSubtype, hasSuffix } from "media-typer";

fn inspectHeader(raw: string): bool {
    let mt = parse(raw);
    if (!mt.isValid()) { return false; }
    // 例如 "application/vnd.api+json" -> main_type="application", subtype="vnd.api", suffix="json"
    return isVendorSubtype(mt.subtype) && hasSuffix(raw, "json");
}
```

### 2. 规范小写格式化与等价判断

```typescript
import { parse, format, mediaTypeEq } from "media-typer";

fn canonicalize(raw: string, out: []u8): usize {
    let mt = parse(raw);
    let n = format(mt, out);
    // "IMAGE/SVG+XML" -> "image/svg+xml"
    let _ = mediaTypeEq(raw, "image/svg+xml"); // true
    return n;
}
```

## 与源包差异与 CP 安全亮点

- **零拷贝解析**：`parse` 返回的 `main_type`/`subtype`/`suffix` 直接切片自 `raw`，非法输入不抛异常而通过 `mt.valid == false` 表示。
- **调用方缓冲输出**：`format`、`formatMediaType`、`essence` 与 `formatTo` 直接写入 `out: []u8` 并返回字节数。
- **丰富语义判定**：内置标准顶级类型与 `vnd.`/`prs.`/`x.` 树判定，基于 `unicase` 实现零开销 ASCII 大小写折叠比较。

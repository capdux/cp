[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# humanize-string

纯 CP 实现的零分配 `camelCase` / `PascalCase` / `snake_case` / `kebab-case` 自然语句转换库，组合复用 `decamelize`。

- **源包对齐**：对标 `npm` `humanize-string` `v3`（MIT，sindresorhus/humanize-string）
- **包版本**：`1.0.0`
- **内存模型**：全程零堆分配，转换结果直接写入调用方提供的 `[]u8` 缓冲区。

## 安装与导入

```bash
cap add humanize-string
```

```cp
import {
    HumanizeOptions,
    humanizeString,
    humanizeStringWithOptions,
    titleizeString,
    humanizeLength,
} from "humanize-string";
```

## 核心 API

| 符号 | 签名 | 说明 |
| --- | --- | --- |
| `HumanizeOptions` | `class HumanizeOptions { capitalize_first: bool, preserve_acronyms: bool }` | 转换选项；`HumanizeOptions.defaults()` 默认首字母大写（`true`）、不保留缩写大写（`false`）。 |
| `humanizeString` | `fn humanizeString(input: string, out: []u8): usize` | 将 `input` 转换为自然语句（首字母大写、单词间单空格）写入 `out`，返回写入字节数。 |
| `humanizeStringWithOptions` | `fn humanizeStringWithOptions(input: string, opts: HumanizeOptions, out: []u8): usize` | 按 `opts` 配置执行转换并写入 `out`，返回写入字节数。 |
| `titleizeString` | `fn titleizeString(input: string, out: []u8): usize` | 转换并将每个单词首字母大写写入 `out`，返回写入字节数。 |
| `humanizeLength` | `fn humanizeLength(input: string): usize` | 预计算 `humanizeString` 的精确输出字节长度，无需缓冲区。 |

## 使用示例

### 1. 基础转换与标题化

```cp
import { humanizeString, titleizeString } from "humanize-string";

fn basicDemo(): void {
    let mut buf: [64]u8 = [0; 64];
    const n1 = humanizeString("fooBar_baz-qux", buf[0..]);
    // buf[0..n1] == "Foo bar baz qux"

    const n2 = titleizeString("http_status_code", buf[0..]);
    // buf[0..n2] == "Http Status Code"
}
```

### 2. 自定义选项（保留缩写 / 控制首字母大小写）

```cp
import { HumanizeOptions, humanizeStringWithOptions, humanizeLength } from "humanize-string";

fn optionsDemo(): void {
    const src = "parseHTMLDocument";
    const needed = humanizeLength(src);

    let mut opts = HumanizeOptions.defaults();
    opts.preserve_acronyms = true;
    opts.capitalize_first = true;

    let mut buf: [64]u8 = [0; 64];
    const n = humanizeStringWithOptions(src, opts, buf[0..]);
    // buf[0..n] == "Parse HTML document"
}
```

## 与源包差异与安全亮点

- **零堆分配缓冲写入**：直接写入调用方 `out: []u8`，避免正则表达式与中间字符串堆分配，并支持 `humanizeLength` 预估精确长度。
- **灵活选项扩展**：在兼容 `npm humanize-string` v3 默认行为的同时，新增 `HumanizeOptions` 与 `titleizeString`。
- **组合 `decamelize`**：底层复用 `decamelize` 处理驼峰词边界拆分。

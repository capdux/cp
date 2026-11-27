[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# decamelize

面向 CP 语言的零堆分配、单趟 ASCII 驼峰拆分转换器，将 `camelCase` / `PascalCase` 转为自定义分隔符小写字符串（或 `CONSTANT_CASE`），对齐 npm [`decamelize`](https://github.com/sindresorhus/decamelize)（周下载 1.2 亿+）。

## 源包对齐说明

- **上游源包**：npm `decamelize`（`sindresorhus/decamelize`，MIT）
- **CP 实现**：`decamelize` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：全程零堆分配，支持自定义分隔符、连续大写缩写保留（`preserve_consecutive_uppercase`）、精确输出长度预计算，以及 `snake_case` / `kebab-case` / 空格分隔 / `CONSTANT_CASE` 快捷函数。

## 安装与导入

```bash
cap add decamelize
```

```cp
import {
    DecamelizeOptions,
    decamelizedLen,
    decamelizeWithOptions,
    decamelizeOpts,
    decamelize,
    decamelizeDefault,
    toSnakeCase,
    toKebabCase,
    toSpaceCase,
    toConstantCase,
} from "decamelize";
```

## 核心 API

### 结构体

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `DecamelizeOptions` | `struct { separator: string, preserve_consecutive_uppercase: bool }` | 配置项结构体；`DecamelizeOptions.defaults()` 返回 `{ separator: "_", preserve_consecutive_uppercase: false }` |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `decamelize` | `fn decamelize(input: string, separator: string, out: []u8): usize` | 使用指定 `separator` 拆分转小写（`preserve_consecutive_uppercase = false`），返回写入字节数 |
| `decamelizeDefault` | `fn decamelizeDefault(input: string, out: []u8): usize` | 使用默认 `"_"` 分隔符转换 |
| `decamelizeWithOptions` | `fn decamelizeWithOptions(input: string, separator: string, preserve_consecutive_uppercase: bool, out: []u8): usize` | 完整转换器，支持保留连续大写缩写（如 `"testURL"` -> `"test_URL"`） |
| `decamelizeOpts` | `fn decamelizeOpts(input: string, opts: DecamelizeOptions, out: []u8): usize` | 接收 `DecamelizeOptions` 的封装入口 |
| `decamelizedLen` | `fn decamelizedLen(input: string, separator: string, preserve_consecutive_uppercase: bool): usize` | 精确计算 `decamelizeWithOptions` 输出字节长度 |
| `toSnakeCase` | `fn toSnakeCase(input: string, out: []u8): usize` | 转为 `snake_case` |
| `toKebabCase` | `fn toKebabCase(input: string, out: []u8): usize` | 转为 `kebab-case` |
| `toSpaceCase` | `fn toSpaceCase(input: string, out: []u8): usize` | 转为 `"space separated"` 空格分隔小写 |
| `toConstantCase` | `fn toConstantCase(input: string, out: []u8): usize` | 转为 `UPPER_SNAKE_CASE` 常量命名 |

## 使用示例

### 1. 常用命名风格转换

```cp
import { decamelize, toKebabCase, toConstantCase } from "decamelize";
import { print } from "cap:io";

pub fn main() {
    let buf: [64]u8 = [0; 64];

    const n1 = decamelize("unicornRainbow", "_", buf[0..64]);
    print(buf[0..n1] as string); // "unicorn_rainbow"

    const n2 = toKebabCase("xmlHttpRequest", buf[0..64]);
    print(buf[0..n2] as string); // "xml-http-request"

    const n3 = toConstantCase("maxRetryCount", buf[0..64]);
    print(buf[0..n3] as string); // "MAX_RETRY_COUNT"
}
```

### 2. 保留连续大写缩写

```cp
import { DecamelizeOptions, decamelizeOpts } from "decamelize";

pub fn formatAcronym(input: string, out: []u8): usize {
    const opts = DecamelizeOptions {
        separator: "_",
        preserve_consecutive_uppercase: true,
    };
    return decamelizeOpts(input, opts, out); // "testURL" -> "test_URL"
}
```

### 3. 写入前预计算长度

```cp
import { decamelizedLen, decamelizeWithOptions } from "decamelize";

pub fn safeDecamelize(input: string, sep: string, out: []u8): usize {
    if (decamelizedLen(input, sep, false) > out.len) {
        return 0;
    }
    return decamelizeWithOptions(input, sep, false, out);
}
```

## 与源包差异与安全亮点

- **零堆分配**：单趟扫描直接写入调用方 `[]u8` 切片，不依赖正则引擎，无临时对象分配。
- **ASCII 标识符优化**：面向 CLI 参数、JSON 字段与环境变量命名场景优化，边界安全防越界。
- **内置常用快捷函数**：直接提供 `toSnakeCase`、`toKebabCase`、`toSpaceCase`、`toConstantCase` 与 `decamelizedLen`。

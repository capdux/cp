[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# humanize-url

面向 CP 语言的零分配、零拷贝 URL 人性化展示工具——去除协议前缀（`http://`、`https://`、`//`）、前导 `www.` 及末尾 `/`，对齐 npm [`humanize-url`](https://github.com/sindresorhus/humanize-url) v3。

## 源包对齐说明

- **上游源包**：npm `humanize-url` v3（`sindresorhus/humanize-url`，MIT）
- **CP 实现**：`humanize-url` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：提供零拷贝切片接口（`humanizeUrlSlice`）和显式缓冲区写入接口（`humanizeUrl`），并支持精确输出长度预计算（`humanizedLen`）。

## 安装与导入

```bash
cap add humanize-url
```

```cp
import {
    isProtocolRelative,
    extractScheme,
    stripProtocol,
    stripWww,
    stripTrailingSlash,
    humanizeUrlSlice,
    humanizedLen,
    humanizeUrl,
} from "humanize-url";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `humanizeUrlSlice` | `fn humanizeUrlSlice(url: string): string` | 零拷贝人性化：去首尾空白、去 `http://`/`https://`/`//`、去 `www.`（保留 `www.com` 等裸域）、去单个末尾 `/` |
| `humanizedLen` | `fn humanizedLen(url: string): usize` | 精确计算人性化后的字节长度 |
| `humanizeUrl` | `fn humanizeUrl(url: string, out: []u8): usize` | 将人性化结果写入 `out`，返回写入字节数 |
| `isProtocolRelative` | `fn isProtocolRelative(url: string): bool` | 判断去除空白后是否以协议相对前缀 `//` 开头 |
| `extractScheme` | `fn extractScheme(url: string): string` | 提取 `://` 之前的协议名切片（如 `"https"`、`"ftp"`），无协议返回 `""` |
| `stripProtocol` | `fn stripProtocol(url: string): string` | 零拷贝去除不区分大小写的 `https://`、`http://` 或 `//` |
| `stripWww` | `fn stripWww(host_and_rest: string): string` | 仅当剩余主机名含 `.` 时去除前导 `www.`（保留 `www.com`） |
| `stripTrailingSlash` | `fn stripTrailingSlash(url: string): string` | 零拷贝去除单个末尾 `/` |

## 使用示例

### 1. 零拷贝 URL 展示格式化

```cp
import { humanizeUrlSlice } from "humanize-url";
import { print } from "cap:io";

pub fn main() {
    print(humanizeUrlSlice("https://www.sindresorhus.com/"));       // "sindresorhus.com"
    print(humanizeUrlSlice("http://example.com:8080/docs/?a=1"));   // "example.com:8080/docs/?a=1"
    print(humanizeUrlSlice("//www.cp-lang.org/guide/"));            // "cp-lang.org/guide"
    print(humanizeUrlSlice("https://www.com/"));                    // "www.com"
}
```

### 2. 预检长度并写入缓冲区

```cp
import { humanizedLen, humanizeUrl } from "humanize-url";

pub fn formatDisplayUrl(raw: string, out: []u8): usize {
    if (humanizedLen(raw) > out.len) {
        return 0;
    }
    return humanizeUrl(raw, out);
}
```

### 3. 组合底层切片函数

```cp
import { extractScheme, isProtocolRelative, stripProtocol, stripWww } from "humanize-url";

pub fn inspectUrl(url: string): string {
    const scheme = extractScheme(url);       // "https" 或 ""
    const proto_rel = isProtocolRelative(url);
    const no_proto = stripProtocol(url);
    return stripWww(no_proto);
}
```

## 与源包差异与安全亮点

- **零拷贝优先**：`humanizeUrlSlice`、`stripProtocol`、`stripWww`、`stripTrailingSlash` 均直接返回输入字符串的只读子切片，零内存分配。
- **空输入安全**：空字符串或全空白字符串安全返回 `""`，不会抛出异常。
- **裸域保护**：对 `https://www.com/` 或 `https://www.com:8080` 自动保留 `www.com`，避免误剥离二级域。

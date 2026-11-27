[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# strip-ansi

面向 CP 语言的零堆分配 ECMA-48 / ANSI 终端转义序列剥离器（支持 CSI、OSC 与 2 字节 ESC 序列）及可见字符计数器，底层复用 `anstyle-parse` 状态机。

## 源包对齐说明

- **上游源包**：npm [`strip-ansi`](https://github.com/chalk/strip-ansi)（MIT）、crates.io [`strip-ansi-escapes`](https://github.com/luser/strip-ansi-escapes)（MIT）、npm [`string-length`](https://github.com/sindresorhus/string-length)（MIT）
- **CP 实现**：`strip-ansi` v1.0.0，纯 CP 净室 MIT 实现，复用 `anstyle-parse` VT100/ANSI 状态机
- **架构特点**：零堆分配、无正则回溯，完整支持 CSI（`\x1b[...m`）、OSC（`\x1b]...\x07` / ST）与 2 字节 ESC 控制序列。

## 安装与导入

```bash
cap add strip-ansi
```

```cp
import {
    hasAnsi,
    strippedLen,
    stripAnsi,
    visibleAsciiCount,
} from "strip-ansi";
```

## 核心 API

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `hasAnsi` | `fn hasAnsi(input: string): bool` | 快速检测 `input` 是否包含任意 ANSI 转义序列（重导出自 `anstyle-parse`） |
| `strippedLen` | `fn strippedLen(input: string): usize` | 计算剥离所有 ANSI 转义序列后的精确可见字节长度 |
| `stripAnsi` | `fn stripAnsi(input: string, out: []u8): usize` | 剥离所有 ANSI 序列并写入 `out`，返回写入字节数；若 `input` 为空或 `out.len < strippedLen(input)` 返回 `0` |
| `visibleAsciiCount` | `fn visibleAsciiCount(input: string): usize` | 统计去除 ANSI 序列后的可见 UTF-8 标量字符数（ASCII 与 UTF-8 首字节） |

## 使用示例

### 1. 快速路径检测 + 剥离 ANSI 颜色码

```cp
import { hasAnsi, strippedLen, stripAnsi } from "strip-ansi";
import { print } from "cap:io";

pub fn main() {
    const colored = "\x1b[31mError:\x1b[0m build failed";
    if (!hasAnsi(colored)) {
        print(colored);
        return;
    }
    let buf: [128]u8 = [0; 128];
    const n = stripAnsi(colored, buf[0..128]);
    print(buf[0..n] as string); // "Error: build failed"
}
```

### 2. 计算终端表格对齐所需的可见字符数

```cp
import { visibleAsciiCount, strippedLen } from "strip-ansi";

pub fn measureColumn(styled: string): usize {
    const bytes = strippedLen(styled);       // 可见 UTF-8 字节长度
    const chars = visibleAsciiCount(styled); // 可见 UTF-8 字符个数
    return chars;
}
```

### 3. 安全检查容量后清洗日志

```cp
import { strippedLen, stripAnsi } from "strip-ansi";

pub fn cleanLogLine(line: string, out: []u8): usize {
    if (strippedLen(line) > out.len) {
        return 0;
    }
    return stripAnsi(line, out);
}
```

## 与源包差异与安全亮点

- **状态机替代正则**：基于 `anstyle-parse` 线性 O(n) 状态机解析，彻底消除复杂转义串下的正则回溯风险。
- **拒绝静默截断**：`stripAnsi` 预先校验 `strippedLen(input) <= out.len`，缓冲区不足时直接返回 `0`，避免输出残缺文本。
- **全程零堆分配**：内部仅使用栈上 `[64]AnsiToken` 缓冲区，适合高频日志与 CLI 渲染管线。

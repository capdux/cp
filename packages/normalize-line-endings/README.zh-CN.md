[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# normalize-line-endings

面向 CP 语言的零堆分配换行符检测、统计与归一化库（`CRLF` / `CR` <-> `LF`），对齐 crates.io [`normalize-line-endings`](https://github.com/derekdreery/normalize-line-endings) v0.3.0。

## 源包对齐说明

- **上游源包**：crates.io `normalize-line-endings` v0.3.0（`derekdreery/normalize-line-endings`，Apache-2.0）
- **CP 实现**：`normalize-line-endings` v0.3.0，纯 CP 净室 Apache-2.0 实现
- **架构特点**：全程零堆分配，提供换行风格检测（`None`/`Lf`/`Crlf`/`Cr`/`Mixed`）、精确长度预计算，以及双向归一化与换行符剥离。

## 安装与导入

```bash
cap add normalize-line-endings
```

```cp
import {
    LineEnding,
    lineEndingStr,
    detectLineEnding,
    countLineBreaks,
    countLines,
    needsNormalization,
    normalizedLen,
    normalizeToLf,
    normalizeToCrlf,
    normalizeToCr,
    normalizeLineEndings,
} from "normalize-line-endings";
```

## 核心 API

### 枚举类型

| 符号 | 成员 | 说明 |
| :--- | :--- | :--- |
| `LineEnding` | `None \| Lf \| Crlf \| Cr \| Mixed` | 检测结果或目标换行风格 |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `lineEndingStr` | `fn lineEndingStr(le: LineEnding): string` | 返回规范换行串（`"\n"`、`"\r\n"`、`"\r"`，`None`/`Mixed` 返回 `""`） |
| `detectLineEnding` | `fn detectLineEnding(input: string): LineEnding` | 检测输入文本的换行风格（`.None`/`.Lf`/`.Crlf`/`.Cr`/`.Mixed`） |
| `countLineBreaks` | `fn countLineBreaks(input: string): usize` | 统计换行符数量（`\r\n` 计 1 次，单独 `\r` 或 `\n` 各计 1 次） |
| `countLines` | `fn countLines(input: string): usize` | 统计逻辑行数（与 Rust `str::lines().count()` 一致：空串为 `0`，末尾换行不额外加行） |
| `needsNormalization` | `fn needsNormalization(input: string, target: LineEnding): bool` | 判断输入中是否存在与 `target` 不一致的换行符 |
| `normalizedLen` | `fn normalizedLen(input: string, target: LineEnding): usize` | 精确计算归一化为 `target` 后的输出字节数 |
| `normalizeToLf` | `fn normalizeToLf(input: string, out: []u8): usize` | 将 `\r\n` 与孤立 `\r` 统一转为 `\n` 并写入 `out`，返回写入字节数 |
| `normalizeToCrlf` | `fn normalizeToCrlf(input: string, out: []u8): usize` | 将所有换行统一转为 Windows `\r\n` 并写入 `out`，返回写入字节数 |
| `normalizeToCr` | `fn normalizeToCr(input: string, out: []u8): usize` | 将所有换行统一转为 `\r` 并写入 `out`，返回写入字节数 |
| `normalizeLineEndings` | `fn normalizeLineEndings(input: string, target: LineEnding, out: []u8): usize` | 按 `target` 分发归一化（`.Lf`/`.Crlf`/`.Cr`，传 `.None` 则移除全部换行） |

## 使用示例

### 1. 快速跳过检测 + 归一化为 LF

```cp
import { LineEnding, needsNormalization, normalizeToLf } from "normalize-line-endings";
import { print } from "cap:io";

pub fn main() {
    const raw = "line1\r\nline2\rline3\n";
    if (!needsNormalization(raw, .Lf)) {
        print(raw);
        return;
    }
    let buf: [128]u8 = [0; 128];
    const n = normalizeToLf(raw, buf[0..128]);
    print(buf[0..n] as string); // "line1\nline2\nline3\n"
}
```

### 2. 转 CRLF 前精确预估缓冲区大小

```cp
import { LineEnding, normalizedLen, normalizeToCrlf } from "normalize-line-endings";

pub fn toWindowsText(input: string, out: []u8): usize {
    const need = normalizedLen(input, .Crlf);
    if (need > out.len) {
        return 0;
    }
    return normalizeToCrlf(input, out);
}
```

### 3. 检测换行风格与统计行数

```cp
import { detectLineEnding, countLineBreaks, countLines } from "normalize-line-endings";

pub fn inspect(text: string): usize {
    const style = detectLineEnding(text);   // .None / .Lf / .Crlf / .Cr / .Mixed
    const breaks = countLineBreaks(text);
    const lines = countLines(text);
    return lines;
}
```

## 与源包差异与安全亮点

- **双向全风格支持**：除转 `LF` 外，还支持转 `CRLF`、转 `CR`、剥离换行（`.None`），并内置 `detectLineEnding`、`countLineBreaks`、`countLines`。
- **零堆分配**：直接写入调用方 `[]u8` 缓冲区，无额外堆内存开销。
- **膨胀安全预检**：转 `CRLF` 时字节数可能增至最多 `2 * input.len`，建议先调用 `normalizedLen(input, .Crlf)` 校验容量。

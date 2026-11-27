[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# is-binary-path

面向 CP 语言的零分配二进制文件路径检测与多媒体/压缩包/可执行文件/字体分类库，对齐 npm [`is-binary-path`](https://github.com/sindresorhus/is-binary-path) v3（周下载 1.07 亿+）与 `binary-extensions`。

## 源包对齐说明

- **上游源包**：npm `is-binary-path` v3.0.0（`sindresorhus/is-binary-path`，MIT）+ `binary-extensions`
- **CP 实现**：`is-binary-path` v3.0.0，纯 CP 净室 MIT 实现
- **架构特点**：全程零堆分配，不区分大小写匹配扩展名，内置 `BinaryCategory` 细粒度分类枚举与便捷谓词函数。

## 安装与导入

```bash
cap add is-binary-path
```

```cp
import {
    BinaryCategory,
    extractExtension,
    classifyBinaryExtension,
    classifyBinaryPath,
    isBinaryExtension,
    isBinaryPath,
    isTextPath,
    isImagePath,
    isMediaPath,
    isArchivePath,
    isExecutablePath,
    isFontPath,
} from "is-binary-path";
```

## 核心 API

### 枚举类型

| 符号 | 成员 | 说明 |
| :--- | :--- | :--- |
| `BinaryCategory` | `None \| Image \| AudioVideo \| Archive \| Executable \| Font \| DocumentData` | 二进制文件大类（`.None` 表示非已知二进制扩展名） |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isBinaryPath` | `fn isBinaryPath(file_path: string): bool` | 判断文件路径是否具有已知二进制扩展名 |
| `isTextPath` | `fn isTextPath(file_path: string): bool` | 判断路径非空且非已知二进制扩展名 |
| `isBinaryExtension` | `fn isBinaryExtension(ext: string): bool` | 判断扩展名（不含前导 `.`，不区分大小写）是否为已知二进制类型 |
| `extractExtension` | `fn extractExtension(file_path: string): string` | 零拷贝提取扩展名切片（不含 `.`，忽略 `.gitignore` 等隐藏文件，兼容 `/` 与 `\`） |
| `classifyBinaryPath` | `fn classifyBinaryPath(file_path: string): BinaryCategory` | 按扩展名将文件路径归类为 `BinaryCategory` |
| `classifyBinaryExtension` | `fn classifyBinaryExtension(ext: string): BinaryCategory` | 按扩展名（不含 `.`，不区分大小写）归类为 `BinaryCategory` |
| `isImagePath` | `fn isImagePath(file_path: string): bool` | 是否为已知二进制图片路径（`.Image`） |
| `isMediaPath` | `fn isMediaPath(file_path: string): bool` | 是否为已知音视频路径（`.AudioVideo`） |
| `isArchivePath` | `fn isArchivePath(file_path: string): bool` | 是否为已知压缩包路径（`.Archive`） |
| `isExecutablePath` | `fn isExecutablePath(file_path: string): bool` | 是否为已知可执行文件/字节码路径（`.Executable`） |
| `isFontPath` | `fn isFontPath(file_path: string): bool` | 是否为已知字体文件路径（`.Font`） |

## 使用示例

### 1. 过滤二进制文件与源码文件

```cp
import { isBinaryPath, isTextPath } from "is-binary-path";

pub fn shouldIndexFile(path: string): bool {
    if (isBinaryPath(path)) {
        return false; // 自动跳过 .png、.zip、.wasm、.pdf、.exe 等
    }
    return isTextPath(path); // "src/main.cp"、".gitignore"、"Makefile" 均返回 true
}
```

### 2. 按文件类别分桶处理

```cp
import { BinaryCategory, classifyBinaryPath } from "is-binary-path";

pub fn assetBucket(path: string): string {
    return switch (classifyBinaryPath(path)) {
        .Image => "images",
        .AudioVideo => "media",
        .Font => "fonts",
        .Archive => "archives",
        .Executable => "bins",
        .DocumentData => "docs",
        .None => "text",
    };
}
```

### 3. 直接提取与校验扩展名

```cp
import { extractExtension, isBinaryExtension } from "is-binary-path";

pub fn inspectExt(path: string): bool {
    const ext = extractExtension(path); // "assets/logo.PNG" -> "PNG"
    return isBinaryExtension(ext);      // true（不区分大小写）
}
```

## 与源包差异与安全亮点

- **零拷贝零分配**：`extractExtension` 返回输入路径的只读子切片，扩展名匹配全程不生成临时小写字符串。
- **内置大类枚举**：在布尔判断外新增 `BinaryCategory` 及 `isImagePath`/`isMediaPath`/`isArchivePath`/`isExecutablePath`/`isFontPath`，适合构建器与静态资源服务。
- **跨平台路径兼容**：同时支持 POSIX `/` 与 Windows `\` 分隔符，正确区分点文件（如 `.env` 扩展名为 `""`）。

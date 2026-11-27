[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# dunce

面向 CP 语言的零堆分配 Windows Verbatim（`\\?\`）与 Verbatim UNC（`\\?\UNC\`）路径安全简化器，对齐 Rust crates.io [`dunce`](https://gitlab.com/kornelski/dunce)。

## 源包对齐说明

- **上游源包**：crates.io `dunce`（`kornelski/dunce`，MIT）
- **CP 实现**：`dunce` v1.0.0，纯 CP 净室 MIT 实现
- **安全保证**：仅当简化后的路径完全满足传统 Win32 路径规则（长度 `<= 259`、无 `.`/`..` 或 `/`、无非法字符 `< > : " | ? *`、无末尾空格或点、无 `CON`/`NUL`/`COM1`..`COM9`/`LPT1`..`LPT9` 等 DOS 保留设备名）时才剥离前缀，否则原样保留。

## 安装与导入

```bash
cap add dunce
```

```cp
import {
    MAX_LEGACY_PATH,
    isIllegalWinChar,
    isWindowsReservedName,
    isSafeLegacyComponent,
    hasWindowsVerbatimPrefix,
    hasWindowsVerbatimUncPrefix,
    canStripVerbatimPrefix,
    canSimplifyUncPath,
    simplifiedSlice,
    simplifiedLen,
    simplified,
    simplifiedUnc,
    simplifiedWithUnc,
} from "dunce";
```

## 核心 API

### 常量

| 符号 | 类型与取值 | 说明 |
| :--- | :--- | :--- |
| `MAX_LEGACY_PATH` | `const MAX_LEGACY_PATH: usize`（`259`） | 传统 Win32 安全路径最大长度（`MAX_PATH - 1`） |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `simplifiedSlice` | `fn simplifiedSlice(path: string): string` | 零拷贝驱动器路径简化（`\\?\C:\...` -> `C:\...`）；不安全、非 verbatim 或 UNC 路径原样返回 |
| `simplifiedLen` | `fn simplifiedLen(path: string): usize` | 精确计算 `simplified(path, out)` 的输出字节数 |
| `simplified` | `fn simplified(path: string, out: []u8): usize` | 将驱动器路径简化写入 `out`（仅在 `canStripVerbatimPrefix` 为真时剥离 `\\?\`），返回写入字节数 |
| `simplifiedUnc` | `fn simplifiedUnc(path: string, out: []u8): usize` | 安全时将 `\\?\UNC\server\share...` 简化为 `\\server\share...`，否则原样拷贝 |
| `simplifiedWithUnc` | `fn simplifiedWithUnc(path: string, out: []u8): usize` | 统一支持驱动器路径与 UNC 路径的完整简化入口 |
| `canStripVerbatimPrefix` | `fn canStripVerbatimPrefix(path: string): bool` | 判断从驱动器路径剥离 `\\?\` 是否完全安全 |
| `canSimplifyUncPath` | `fn canSimplifyUncPath(path: string): bool` | 判断 `\\?\UNC\server\share...` 是否可安全简化为 `\\server\share...` |
| `hasWindowsVerbatimPrefix` | `fn hasWindowsVerbatimPrefix(path: string): bool` | 判断路径是否以 `\\?\` 开头 |
| `hasWindowsVerbatimUncPrefix` | `fn hasWindowsVerbatimUncPrefix(path: string): bool` | 判断路径是否以 `\\?\UNC\` 开头（`UNC` 不区分大小写） |
| `isSafeLegacyComponent` | `fn isSafeLegacyComponent(component: string): bool` | 校验单个路径段是否符合传统 Win32 安全规则 |
| `isWindowsReservedName` | `fn isWindowsReservedName(component: string): bool` | 不区分大小写检测 DOS 保留设备名（`CON`/`PRN`/`AUX`/`NUL`/`COM1`..`COM9`/`LPT1`..`LPT9`，含 `CON.txt`） |
| `isIllegalWinChar` | `fn isIllegalWinChar(ch: u8): bool` | 判断字节是否为 Windows 非法路径字符（`< > : " \| ? * /` 或控制字符） |

## 使用示例

### 1. 零拷贝驱动器路径简化

```cp
import { simplifiedSlice } from "dunce";
import { print } from "cap:io";

pub fn main() {
    print(simplifiedSlice("\\\\?\\C:\\Users\\alice\\doc.txt")); // "C:\Users\alice\doc.txt"
    print(simplifiedSlice("\\\\?\\C:\\CON.txt"));                // "\\?\C:\CON.txt"（安全保留）
}
```

### 2. 驱动器 + UNC 路径统一简化

```cp
import { simplifiedWithUnc } from "dunce";

pub fn normalizeWinPath(path: string, out: []u8): usize {
    return simplifiedWithUnc(path, out);
    // "\\?\UNC\filesrv\share\a.txt" -> "\\filesrv\share\a.txt"
    // "\\?\D:\work\repo"            -> "D:\work\repo"
}
```

### 3. 路径段安全校验

```cp
import { isSafeLegacyComponent, isWindowsReservedName } from "dunce";

pub fn checkName(name: string): bool {
    if (isWindowsReservedName(name)) {
        return false;
    }
    return isSafeLegacyComponent(name);
}
```

## 与源包差异与安全亮点

- **纯路径分析无 I/O**：不调用系统 `canonicalize` 文件系统接口，跨平台确定性执行，无副作用。
- **零拷贝快速路径**：`simplifiedSlice` 直接返回输入路径的子切片，零堆分配与零拷贝。
- **完整 UNC 支持**：`simplifiedUnc` / `simplifiedWithUnc` 同样执行长度上限（`<= 259`）与保留设备名检查，避免生成破坏性路径。

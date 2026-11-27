[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# semver-diff

面向 CP 语言的零分配 SemVer 升级差异分类器（`major`、`minor`、`patch`、`premajor`、`preminor`、`prepatch`、`prerelease`、`build`）与破坏性升级检测器，对齐 npm [`semver-diff`](https://github.com/sindresorhus/semver-diff) v5。

## 源包对齐说明

- **上游源包**：npm `semver-diff` v5（`sindresorhus/semver-diff`，MIT）
- **CP 实现**：`semver-diff` v1.0.0，纯 CP 净室 MIT 实现，底层复用 `semver` 解析与比较引擎
- **架构特点**：零堆分配；同时提供强类型枚举 `SemverDiffKind`、紧凑 `u8` 常量 `DIFF_*` 以及可选静态字符串 `string?` 三种接口。

## 安装与导入

```bash
cap add semver-diff
```

```cp
import {
    DIFF_NONE,
    DIFF_MAJOR,
    DIFF_MINOR,
    DIFF_PATCH,
    DIFF_PREMAJOR,
    DIFF_PREMINOR,
    DIFF_PREPATCH,
    DIFF_PRERELEASE,
    DIFF_BUILD,
    SemverDiffKind,
    diffKindFromCode,
    diffKindToCode,
    semverDiffName,
    semverDiffNameOrEmpty,
    semverDiffKind,
    semverDiffEnum,
    semverDiff,
    isUpgrade,
    isPrereleaseDiff,
    compareDiffSeverity,
    isBreakingUpgrade,
} from "semver-diff";
```

## 核心 API

### 常量与枚举

| 符号 | 取值 | 说明 |
| :--- | :--- | :--- |
| `DIFF_NONE` .. `DIFF_BUILD` | `0`（`NONE`）、`1`（`MAJOR`）、`2`（`MINOR`）、`3`（`PATCH`）、`4`（`PREMAJOR`）、`5`（`PREMINOR`）、`6`（`PREPATCH`）、`7`（`PRERELEASE`）、`8`（`BUILD`） | 紧凑 `u8` 差异类型编码 |
| `SemverDiffKind` | `None \| Major \| Premajor \| Minor \| Preminor \| Patch \| Prepatch \| Prerelease \| Build` | 强类型 SemVer 升级差异枚举 |

### 函数列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `semverDiff` | `fn semverDiff(ver_a: string, ver_b: string): string?` | 返回规范升级类型名（如 `"major"`、`"preminor"`、`"build"`），非升级或无效版本返回 `null` |
| `semverDiffEnum` | `fn semverDiffEnum(ver_a: string, ver_b: string): SemverDiffKind` | 返回强类型 `SemverDiffKind`（相等、降级或非法版本返回 `.None`） |
| `semverDiffKind` | `fn semverDiffKind(ver_a: string, ver_b: string): u8` | 返回 `DIFF_*` 编码常量（非升级返回 `DIFF_NONE`） |
| `semverDiffName` | `fn semverDiffName(kind: SemverDiffKind): string?` | 将 `SemverDiffKind` 转为静态字符串名，`.None` 返回 `null` |
| `semverDiffNameOrEmpty` | `fn semverDiffNameOrEmpty(kind: SemverDiffKind): string` | 将 `SemverDiffKind` 转为静态字符串名，`.None` 返回 `""` |
| `diffKindFromCode` | `fn diffKindFromCode(code: u8): SemverDiffKind` | 将 `DIFF_*` 编码转为 `SemverDiffKind` |
| `diffKindToCode` | `fn diffKindToCode(kind: SemverDiffKind): u8` | 将 `SemverDiffKind` 转为 `DIFF_*` 编码 |
| `isUpgrade` | `fn isUpgrade(ver_a: string, ver_b: string): bool` | 当 `ver_b` 为 `ver_a` 的有效升级（含 build 元数据变更）时返回 `true` |
| `isBreakingUpgrade` | `fn isBreakingUpgrade(ver_a: string, ver_b: string): bool` | 判断是否为破坏性升级（主版本升级或 `0.x` 次版本升级） |
| `isPrereleaseDiff` | `fn isPrereleaseDiff(kind: SemverDiffKind): bool` | 判断是否为预发布差异（`Premajor`/`Preminor`/`Prepatch`/`Prerelease`） |
| `compareDiffSeverity` | `fn compareDiffSeverity(a: SemverDiffKind, b: SemverDiffKind): i32` | 比较升级严重级别（`-1`/`0`/`1`，`Major` 最高，`None` 最低） |

## 使用示例

### 1. 基础升级差异识别

```cp
import { semverDiff } from "semver-diff";
import { print } from "cap:io";

pub fn main() {
    if (const diff = semverDiff("1.2.3", "2.0.0")) {
        print(diff); // "major"
    }
    if (const pre = semverDiff("1.2.3", "1.3.0-alpha.1")) {
        print(pre);  // "preminor"
    }
    const down = semverDiff("2.0.0", "1.9.9"); // null（降级）
}
```

### 2. 强类型枚举匹配与自动更新策略

```cp
import { SemverDiffKind, semverDiffEnum, isBreakingUpgrade, isPrereleaseDiff } from "semver-diff";

pub fn shouldAutoUpdate(current: string, latest: string): bool {
    if (isBreakingUpgrade(current, latest)) {
        return false;
    }
    const kind = semverDiffEnum(current, latest);
    if (isPrereleaseDiff(kind)) {
        return false;
    }
    return kind == .Minor || kind == .Patch;
}
```

### 3. 依赖升级严重程度排序

```cp
import { semverDiffEnum, compareDiffSeverity } from "semver-diff";

pub fn isMoreSevere(a1: string, b1: string, a2: string, b2: string): bool {
    const k1 = semverDiffEnum(a1, b1);
    const k2 = semverDiffEnum(a2, b2);
    return compareDiffSeverity(k1, k2) > 0;
}
```

## 与源包差异与安全亮点

- **非法输入零异常**：解析失败时安全返回 `.None` / `DIFF_NONE` / `null`，无需异常处理。
- **静态字符串零拷贝**：`semverDiff` 与 `semverDiffName` 直接返回编译期静态字面量，无堆分配与生命周期负担。
- **工程增强工具**：额外内置 `isUpgrade`、`isBreakingUpgrade`、`isPrereleaseDiff`、`compareDiffSeverity`，适配包管理器与升级通知器场景。

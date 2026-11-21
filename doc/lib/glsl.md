# `cap:glsl` — 纯 CP OpenGL / Vulkan GLSL 着色器零分配词法与声明扫描库

## 概述

`cap:glsl` 提供纯 CP、零外部依赖、零堆分配的 OpenGL / Vulkan GLSL 着色器词法扫描器、`#version` 指令解析器、顶层 `uniform`/`in`/`out` 声明（含 `layout(location=N, binding=M)`）提取器与注释感知分隔符平衡校验器，与 `cap:wgsl` 配套。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `GlslVersion` | `#version` 元数据：`number: u32`, `profile: []const u8` |
| `GlslQualifier` | 顶层限定符枚举：`uniform`, `input`, `output` |
| `GlslDecl` | 顶层变量声明：`qualifier`, `ty`, `name`, `location: u32?`, `binding: u32?` |
| `GlslTokenKind` | 词法单元类型：`ident`, `number`, `preproc`, `punct`, `invalid` |
| `GlslToken` | 零拷贝词法单元：`kind`, `lexeme` |
| `GlslIter` | 流式词法扫描器：`init(input)`、`next(*_): GlslToken?` |
| `parseGlslVersion(input)` | 解析首个 `#version <num> [profile]` 指令 |
| `GlslDeclIter` | 顶层 `[layout(location=N, binding=M)] (uniform|in|out) <type> <name>;` 声明迭代器 |
| `validateGlslBraces(input)` | 注释感知校验 `{}`/`()`/`[]` 括号平衡与块注释闭合 |

## 示例

```cp
import { parseGlslVersion, GlslDeclIter, validateGlslBraces } from "cap:glsl";

fn example(src: []const u8): bool {
    if (!validateGlslBraces(src)) return false;
    const ver = parseGlslVersion(src) ?? return false;
    let it = GlslDeclIter.init(src);
    return ver.number >= 330 && it.next() != null;
}
```

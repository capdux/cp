# `cap:wgsl` — 纯 CP WebGPU WGSL 着色器零分配词法扫描与入口点/绑定提取库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:compiler`，导入请使用 `import { ... } from "cap:compiler"`。

## 概述

`cap:wgsl` 提供纯 CP、零外部依赖、零堆分配的 W3C WebGPU Shading Language (`.wgsl`) 词法扫描器（支持 `//` 行注释与 WGSL §2.2 可嵌套 `/* ... */` 块注释）、`@vertex`/`@fragment`/`@compute` 入口点扫描器、`@group(g) @binding(b) var... <name>` 资源绑定提取器与分隔符平衡校验器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `WgslStage` | 着色器阶段枚举：`vertex`, `fragment`, `compute` |
| `WgslEntryPoint` | 入口点元数据：`stage`, `name` |
| `WgslBinding` | 资源绑定元数据：`group: u32`, `binding: u32`, `var_name` |
| `WgslTokenKind` | 词法单元类型：`ident`, `number`, `at_attr`, `punct`, `invalid` |
| `WgslToken` | 零拷贝词法单元：`kind`, `lexeme` |
| `WgslIter` | 流式词法扫描器：`init(input)`、`next(*_): WgslToken?` |
| `WgslEntryPointIter` | 入口点扫描器：`init(input)`、`next(*_): WgslEntryPoint?` |
| `WgslBindingIter` | 资源绑定扫描器：`init(input)`、`next(*_): WgslBinding?` |
| `validateWgslDelimiters(input)` | 校验 `{}`/`()`/`[]` 括号平衡与嵌套 `/* ... */` 块注释闭合 |

## 示例

```cp
import { WgslEntryPointIter, WgslBindingIter, validateWgslDelimiters } from "cap:compiler";

fn example(shader: []const u8): bool {
    if (!validateWgslDelimiters(shader)) return false;
    let eps = WgslEntryPointIter.init(shader);
    let binds = WgslBindingIter.init(shader);
    return eps.next() != null && binds.next() != null;
}
```

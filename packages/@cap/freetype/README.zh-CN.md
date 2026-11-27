[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/freetype

FreeType 2.13.3 industrial-standard TrueType/SFNT font rasterization engine for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/freetype` |
| 版本 | `2.13.3` |
| 许可证 | `FTL` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/freetype
```

```cp
import {
    FreeTypeError,
    FtVersion,
    FtFace,
    FtLibrary,
    cp_ft_init,
    cp_ft_done,
    cp_ft_version,
    cp_ft_new_memory_face,
} from "@cap/freetype";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `FreeTypeError` | `InitFailed, InvalidState, FaceLoadFailed` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `FtVersion` | `class` | 字段: `major: i32, minor: i32, patch: i32` |
| `FtFace` | `class` | 字段: `raw: *auto!?` · 方法: `numGlyphs, __cp_deinit` |
| `FtLibrary` | `class` | 字段: `raw: *auto!?` · 方法: `init, version, newMemoryFace, __cp_deinit` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `cp_ft_init` | `(): *auto!?` | 调用 `cp_ft_init` 并返回 `*auto!?`。 |
| `cp_ft_done` | `(lib: *auto!?)` | 就地执行 `cp_ft_done` 操作。 |
| `cp_ft_version` | `(lib: *auto!?, maj: *i32!, min: *i32!, pat: *i32!)` | 就地执行 `cp_ft_version` 操作。 |
| `cp_ft_new_memory_face` | `(lib: *auto!?, data: [*]&u8!, size: i64, face_idx: i64): *auto!?` | 调用 `cp_ft_new_memory_face` 并返回 `*auto!?`。 |
| `cp_ft_done_face` | `(face: *auto!?)` | 就地执行 `cp_ft_done_face` 操作。 |
| `cp_ft_face_num_glyphs` | `(face: *auto!?): i64` | 调用 `cp_ft_face_num_glyphs` 并返回 `i64`。 |

## 最小调用示例

```cp
import { cp_ft_init } from "@cap/freetype";

pub fn main() {
    let result = cp_ft_init();
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。

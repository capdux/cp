# `cap:macho` — 纯 CP Apple Mach-O 32/64-bit 与 Fat Universal 二进制解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

> 参考来源：`/data/dev/bun/src/exe_format/macho.cp`, `/data/dev/bun/src/standalone_module_graph.zig` | 吸收日期：2026-11-22

## 概述

`cap:macho` 提供纯 CP、零外部依赖、零堆分配的 Apple Mach-O（Thin 32/64-bit，支持大端与小端字节序）及 Fat Universal Binary（32/64-bit）二进制魔数探测、头部解码、Load Command 安全迭代与架构切片列表解析器，全面防护畸形 `cmdsize` 与缓冲区越界。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `MachoKind` | 二进制格式枚举：`thin32`, `thin64`, `fat32`, `fat64`, `unknown` |
| `MachoHeader` | 头部元数据：`is_64bit`, `is_big_endian`, `cputype`, `cpusubtype`, `filetype`, `ncmds`, `sizeofcmds`, `flags`；方法：`isArm64()`, `isX86_64()`, `isExecutable()`, `isDylib()`, `headerSize()` |
| `MachoLoadCmd` | 零拷贝 Load Command：`cmd: u32`, `cmdsize: u32`, `payload: []const u8` |
| `FatArchEntry` | Fat 架构条目：`cputype`, `cpusubtype`, `offset: u64`, `size: u64`, `align_pow2: u32` |
| `detectMachoKind(data)` | 识别 8 种标准 Mach-O/Fat 魔数 |
| `parseMachoHeader(data)` | 校验头部长度并按字节序解码 `MachoHeader` |
| `MachoLoadCmdIter` | 流式 Load Command 迭代器：`init(data)`、`next(*_): MachoLoadCmd?`（严格校验 `cmdsize >= 8` 与边界） |
| `FatArchIter` | Fat Universal 架构列表迭代器：`init(data)`、`next(*_): FatArchEntry?` |

## 示例

```cp
import { detectMachoKind, parseMachoHeader, MachoLoadCmdIter, MachoKind } from "cap:binfmt";

fn isArm64Executable(bin: []const u8): bool {
    if (detectMachoKind(bin) != MachoKind.thin64) return false;
    if (hdr := parseMachoHeader(bin)) {
        return hdr.isArm64() && hdr.isExecutable();
    }
    return false;
}
```

---

## 上游行为契约与安全边界注意事项

1. **大端/小端字节序与 8 种标准魔数区分**：
   - `detectMachoKind` 严格区分 32/64-bit Thin Mach-O（`0xFEEDFACE`/`0xFEEDFACF` 及其反字节序）与 32/64-bit Fat Universal Binary（`0xCAFEBABE`/`0xCAFEBABF` 及其反字节序）；`parseMachoHeader` 自动按探测到的字节序解码全部头部字段，注意 Fat 头在 macOS 规范中始终按大端序编码。
2. **畸形 Load Command 防护（`MachoLoadCmdIter`）**：
   - 每步迭代严格校验 `cmdsize >= 8` 且 `offset + cmdsize <= headerSize + sizeofcmds <= data.len`，彻底防止 `cmdsize == 0` 触发的无限循环或越界切片访问。
3. **Fat Universal Binary 切片提取（`FatArchIter`）**：
   - 在 macOS 通用二进制（同时包含 `CPU_TYPE_ARM64` 与 `CPU_TYPE_X86_64`）中，通过 `FatArchIter` 遍历每个架构条目的 `offset` 与 `size`，校验范围合法后再将子切片 `bin[offset .. offset + size]` 递归送入 `parseMachoHeader` 解析具体架构段。

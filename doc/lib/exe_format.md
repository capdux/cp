# `cap:exe_format` — 可执行二进制格式零拷贝探测与解析库

## 1. 概述

`cap:exe_format`（`cap.exe_format`）提供纯 CP 实现的工业级可执行二进制格式零拷贝探测、头部校验、节区解析与自包含尾部载荷（Standalone Trailer）读写能力，支持 **ELF64 / ELF32**、**Mach-O 64 / Fat**、**PE32+ / PE32 (PE-COFF)**、**WebAssembly** 与 **Shebang** 脚本。所有解析路径均执行严格整数溢出与切片边界校验，有效拦截截断与畸形头部。

## 2. 核心类型

- `ExeKind`: `Elf64 | Elf32 | MachO64 | MachOFat | Pe32Plus | Pe32 | Wasm | Shebang | Unknown`
- `ExeArch`: `Unknown | X86_64 | X86 | Aarch64 | Arm | Riscv64 | Wasm32 | Universal`
- `Elf64Header`: ELF64 文件头解析结果（含 `e_type`、`e_machine`、`e_entry`、`e_shoff`、`e_shentsize`、`e_shnum`、`e_shstrndx`、`arch`）
- `Elf64Section`: 零拷贝节区视图（`name`、`sh_type`、`flags`、`addr`、`offset`、`size`、`data`）
- `PeHeaderInfo`: PE-COFF 头部信息（`is_pe32_plus`、`machine`、`arch`、`num_sections`、`pe_offset`、`sections_offset`、`entry_point_rva`、`image_base`）
- `MachO64HeaderInfo`: Mach-O 64 头部信息（`magic`、`cputype`、`cpusubtype`、`filetype`、`ncmds`、`sizeofcmds`、`flags`、`arch`）
- `StandaloneTrailer`: 尾部载荷零拷贝视图（`payload`、`payload_len`、`checksum`、`magic`、`total_len`）

## 3. API 速查

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `detectExeKind` | `(bytes: []const u8): ExeKind` | 零拷贝魔数与格式探测 |
| `parseElf64Header` | `(bytes: []const u8): Elf64Header!` | 解析并校验 ELF64 文件头与节区表边界 |
| `listElf64Sections` | `(alloc: Allocator, bytes: []const u8): []Elf64Section!` | 列出全部 ELF64 节区并解析 `.shstrtab` 节名 |
| `findElf64Section` | `(bytes: []const u8, name: []const u8): Elf64Section?!` | 零分配按名称查找 ELF64 节区 |
| `parsePeHeader` | `(bytes: []const u8): PeHeaderInfo!` | 解析并校验 DOS/PE-COFF/OptionalHeader |
| `parseMachO64Header` | `(bytes: []const u8): MachO64HeaderInfo!` | 解析并校验 Mach-O 64 头与 Load Commands 边界 |
| `appendStandaloneTrailer` | `(alloc: Allocator, exe_bytes: []const u8, payload: []const u8, magic8: [8]u8): []u8!` | 追加带 FNV-1a 64 位校验和的尾部载荷 |
| `extractStandaloneTrailer` | `(bytes: []const u8, magic8: [8]u8): StandaloneTrailer!` | 零拷贝提取并校验尾部载荷 |

## 4. 示例

```typescript
import { detectExeKind, parseElf64Header, findElf64Section } from "cap:exe_format";

const kind = detectExeKind(bin_bytes);
if (kind == .Elf64) {
    const hdr = parseElf64Header(bin_bytes)?;
    const sec = findElf64Section(bin_bytes, ".cp_bundle")?;
}
```

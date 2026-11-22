# `cap:elf` — 跨平台可执行二进制格式嗅探与完整 ELF64 / Mach-O / PE 解析标准库

> 参考来源：`/data/dev/bun/src/exe_format/elf.cp`, `/data/dev/bun/src/exe_format/macho.cp`, `/data/dev/bun/src/exe_format/pe.cp` | 吸收日期：2026-11-22
> **模块路径**: `lib/elf/`  
> **导入协议**: `import { ExeFormat, ExeFormatKind, ElfClass, ElfEndian, ElfType, ElfMachine, Elf64HeaderInfo, Elf64Phdr, Elf64Shdr, Elf64Sym, Elf64Dyn, Elf64File, Elf64Parser, ExeInspector, MachHeader64, MachoInspector, PeCoffHeader, PeInspector } from "cap:elf";`

---

## 一、 架构概述与文件结构

`cap:elf` 吸收自 Bun 的 `src/exe_format/`，提供零外部头文件依赖的多格式魔数识别、完整 ELF64 结构解析（Header / Program Header / Section Header / Symbol Table / Dynamic Section），以及 Mach-O 64 与 Windows PE/COFF 帧头解析：

```text
lib/elf/
├── index.cp       # 统一公开导出入口
├── format.cp      # 多格式魔数识别 (ExeFormat.detect / ExeInspector.detectFormat) 与 ELF64 Header 解析
├── elf64.cp       # 完整 ELF64 解析器 (Elf64File, Elf64Phdr, Elf64Shdr, Elf64Sym, Elf64Dyn)
└── machoPe.cp    # Mach-O 64-bit 与 PE/COFF 帧头解析器
```

### 1. 多格式魔数识别 (`ExeFormat.detect` & `ExeInspector.detectFormat`)

| `ExeFormat` 枚举 | 魔数字节特征 | 说明 |
| :--- | :--- | :--- |
| `Elf32` / `Elf64` | `\x7FELF` + `EI_CLASS` (`1` 或 `2`) | Linux / BSD 可执行文件、共享库与目标文件 |
| `MachO32` / `MachO64` | `0xFEEDFACE` / `0xFEEDFACF` (LE/BE) | macOS / iOS 32/64 位 Mach-O |
| `MachOFat` | `0xCAFEBABE` / `0xBEBAFECA` | macOS Universal Binary 胖二进制 |
| `Pe` | `MZ` + `e_lfanew -> "PE\0\0"` | Windows PE/COFF 可执行文件与 DLL |
| `Wasm` | `\x00asm` | WebAssembly 模块 |
| `Shebang` | `#!` | Unix 脚本解释器头 |

### 2. ELF64 完整解析器 (`Elf64File`)

- **ELF64 Header (`Elf64HeaderInfo`)**：支持小端与大端 (`EI_DATA = 1/2`)，`ElfType` (`ET_REL`/`ET_EXEC`/`ET_DYN`/`ET_CORE`)、`ElfMachine` (`X86`/`X86_64`/`AArch64`/`RiscV`/`Arm`/`Ppc64`)、入口地址、PH/SH 偏移及条目尺寸。
- **Program Header (`Elf64Phdr`)**：解析 `PT_LOAD`、`PT_DYNAMIC`、`PT_INTERP`、`PT_NOTE`、`PT_GNU_STACK`、`PT_GNU_RELRO` 与 `PF_R`/`PF_W`/`PF_X` 权限；`elf.interpreter()` 直接提取动态链接器路径（如 `/lib64/ld-linux-x86-64.so.2`）。
- **Section Header (`Elf64Shdr`)**：支持按索引或名称查找节 (`findSection(name)`)、读取原始字节切片 (`sectionData` / `findSectionData`)，识别 `SHT_PROGBITS`、`SHT_SYMTAB`、`SHT_STRTAB`、`SHT_DYNSYM`、`SHT_DYNAMIC`、`SHT_RELA` 等。
- **Symbol Table (`Elf64Sym`)**：解析 `.symtab` 与 `.dynsym`，提取 `bind()` (`LOCAL`/`GLOBAL`/`WEAK`)、`symType()` (`NOTYPE`/`OBJECT`/`FUNC`/`SECTION`/`FILE`/`TLS`)、`visibility()` (`DEFAULT`/`INTERNAL`/`HIDDEN`/`PROTECTED`) 并关联 `.strtab`/`.dynstr` 解析符号名 (`findSymbol`)。
- **Dynamic Section (`Elf64Dyn`)**：解析 `.dynamic` / `PT_DYNAMIC`，提取 `DT_NEEDED` 动态库列表 (`getNeededLibs`)、`DT_SONAME` (`soname()`)、`DT_RPATH` (`rpath()`)、`DT_RUNPATH` (`runpath()`)。

---

## 二、 代码示例

### 解析完整 ELF64 内存镜像或真实 Linux 二进制

```typescript
import {
    ExeFormat,
    Elf64File,
    PT_LOAD,
} from "cap:elf";

fn inspectBinary(bytes: []const u8): void {
    if (ExeFormat.detect(bytes) != ExeFormat.Elf64) return;
    const elf = Elf64File.parse(bytes) ?? return;

    // 1. 查看入口与解释器
    const entry = elf.header().entry_point;
    const interp = elf.interpreter() ?? "";

    // 2. 按名称查找 .text 节数据
    if (text_bytes := elf.findSectionData(".text")) {
        * = text_bytes;
    }

    // 3. 查找符号表中的函数
    if (sym := elf.findSymbol("main", false)) {
        * = sym.st_value;
    }

    // 4. 提取 DT_NEEDED 动态库依赖
    let libs: [16][]const u8 = undefined;
    const n_libs = elf.getNeededLibs(libs[0..16]);
    * = n_libs;
    * = entry;
    * = interp;
    * = elf.findPhdr(PT_LOAD);
}
```

---

## 上游 Bun `exe_format` 行为契约与安全边界注意事项

1. **零拷贝视图与畸形二进制边界防护**：
   - `Elf64File`、`MachoInspector` 与 `PeInspector` 均直接基于只读字节切片进行边界检查与零拷贝字段读取；面对外部不可信或截断的二进制文件，所有 `sh_offset + sh_size`、`p_offset + p_filesz` 与 `e_lfanew` 偏移均经过无溢出范围校验，防止越界切片读取。
2. **动态符号表与常规符号表区分（`findSymbol`）**：
   - `elf.findSymbol(name, true)` 检索 `.dynsym`（剥离后的生产发布二进制仍保留导出动态符号），`findSymbol(name, false)` 检索 `.symtab`（包含本地与调试符号，经 `strip` 后可能不存在）。
3. **跨平台单文件可执行体（Standalone Executable）检测**：
   - 结合 `ExeFormat.detect`、`getNeededLibs`（`DT_NEEDED`）与 `findSectionData`，可用于工具链自检、动态库依赖审计（类 `ldd`/`readelf`）及跨平台分发产物校验。

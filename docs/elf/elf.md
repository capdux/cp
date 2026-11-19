# `cap:elf` — 跨平台可执行二进制格式嗅探与 ELF64 / Mach-O / PE 帧头解析标准库

> **上游溯源**: `/data/dev/bun/src/exe_format/` (`elf.cp`, `macho.cp`, `pe.cp`)  
> **模块路径**: `lib/elf/`  
> **导入协议**: `import { ExeFormatKind, ElfMachine, Elf64HeaderInfo, ExeInspector, MachHeader64, MachoInspector, PeCoffHeader, PeInspector } from "cap:elf";`

---

## 一、 架构概述与二进制帧头布局

`cap:elf` 吸收自 Bun 的跨平台可执行文件分析器（`bun/src/exe_format`），无需依赖平台特定的系统头文件，即可在任意宿主系统上对 **Linux ELF64**、**macOS Mach-O 64 / Universal Fat**、**Windows PE/COFF** 以及 **WebAssembly (`.wasm`)** 二进制镜像进行零堆分配魔数嗅探与帧头结构化解析：

### 1. 五大二进制格式魔数识别矩阵 (`ExeInspector.detectFormat`)

| 格式枚举 (`ExeFormatKind`) | 起始偏移与魔数字节序列 (Magic Bytes) | 适用平台与二进制类型 |
| :--- | :--- | :--- |
| `ExeFormatKind.Elf` | `0x7F, 'E', 'L', 'F'` (`\x7FELF`) | Linux / FreeBSD 可执行文件、共享库 (`.so`)、目标文件 (`.o`) |
| `ExeFormatKind.MachO64` | `0xCF, 0xFA, 0xED, 0xFE` (LE) 或 `0xFE, 0xED, 0xFA, 0xCF` (BE) | macOS / iOS 64 位原生可执行文件与动态库 (`.dylib`) |
| `ExeFormatKind.MachOFat` | `0xCA, 0xFE, 0xBA, 0xBE` | macOS Universal Binary 多架构胖二进制容器 (`x86_64` + `arm64`) |
| `ExeFormatKind.Pe` | `'M', 'Z'` (`0x4D, 0x5A`) + `@offset[60] -> "PE\0\0"` | Windows 可执行文件 (`.exe`) 与动态链接库 (`.dll`) |
| `ExeFormatKind.Wasm` | `0x00, 'a', 's', 'm'` (`\x00asm`) | WebAssembly 二进制模块 (`.wasm`) |

### 2. 三大原生可执行帧头内存布局

1. **ELF64 帧头 (`Elf64_Ehdr` — 64 字节，`ExeInspector.parseElf64Header`)**：
   - `[0..4]`: `\x7FELF` 魔数；`[4]`: `EI_CLASS = 2` (64-bit)；`[5]`: `EI_DATA = 1` (Little-Endian)。
   - `[16..18]`: `e_type` (`u16`，如 `2 = ET_EXEC`, `3 = ET_DYN`)。
   - `[18..20]`: `e_machine` (`u16`，对应 `ElfMachine.X86_64 = 62`, `AArch64 = 183`, `RiscV = 243`)。
   - `[24..32]`: `e_entry` (`u64` 程序入口虚拟地址)。
   - `[32..40]`: `e_phoff` (`u64` Program Header 表偏移)；`[40..48]`: `e_shoff` (`u64` Section Header 表偏移)。
   - `[56..58]`: `e_phnum` (`u16`)；`[60..62]`: `e_shnum` (`u16`)；`[62..64]`: `e_shstrndx` (`u16`)。
2. **Mach-O 64 帧头 (`mach_header_64` — 32 字节，`MachoInspector.parseHeader64` / `encodeHeader64`)**：
   - 包含 `magic` (`0xFEEDFACF`)、`cputype` (`0x01000007` 为 `x86_64`，`0x0100000C` 为 `arm64`)、`cpusubtype`、`filetype` (`2 = MH_EXECUTE`, `6 = MH_DYLIB`)、`ncmds`（Load Commands 数量）、`sizeofcmds`、`flags` 与 `reserved`。
3. **PE/COFF 帧头 (`IMAGE_DOS_HEADER` + `IMAGE_FILE_HEADER` — `PeInspector.parseCoffHeader`)**：
   - 从 DOS 头偏移 `60` (`0x3C`) 读取 32 位 `e_lfanew` 指针，跳转校验 4 字节 `"PE\0\0"` 签名，随后解析 20 字节 COFF 头：`machine` (`0x8664 = AMD64`, `0xAA64 = ARM64`)、`num_sections`、`time_date_stamp`、`size_of_optional_header` 与 `characteristics`。

---

## 二、 核心 API 一览表

| 结构体 | 方法签名 | 功能说明 |
| :--- | :--- | :--- |
| **`ExeInspector`** | `detectFormat(bytes): ExeFormatKind` | 根据前 2~4 字节魔数瞬间判定二进制格式 |
| **`ExeInspector`** | `parseElf64Header(bytes): Elf64HeaderInfo?` | 解析 64 字节小端序 ELF64 头部信息 |
| **`ExeInspector`** | `parsePeSignatureOffset(bytes): u32?` | 校验 DOS `MZ` 头与 `PE\0\0` 签名并返回 `e_lfanew` 偏移 |
| **`MachoInspector`** | `parseHeader64(bytes): MachHeader64?` | 解析 32 字节 `mach_header_64` 结构 |
| **`MachoInspector`** | `encodeHeader64(out, hdr): usize` | 将 `MachHeader64` 编码序列化为 32 字节小端序二进制流 |
| **`PeInspector`** | `parseCoffHeader(bytes): PeCoffHeader?` | 解析 Windows PE/COFF 机器架构、节数量与可选头大小 |

---

## 三、 代码示例

### 1. 跨平台可执行格式嗅探与 ELF64 帧头解析

```typescript
import {
    ExeFormatKind,
    ElfMachine,
    ExeInspector,
} from "cap:elf";

fn demoElfInspection(): void {
    // 1. 格式魔数嗅探
    let fmt_elf  = ExeInspector.detectFormat("\x7FELF\x02\x01");      // ExeFormatKind.Elf
    let fmt_wasm = ExeInspector.detectFormat("\x00asm\x01\x00\x00\x00"); // ExeFormatKind.Wasm

    // 2. 构造并解析 64 字节 ELF64 头部
    let elf_bytes: [64]u8 = [0; 64];
    elf_bytes[0] = 0x7F; elf_bytes[1] = 'E'; elf_bytes[2] = 'L'; elf_bytes[3] = 'F';
    elf_bytes[4] = 2;  // 64-bit
    elf_bytes[5] = 1;  // Little-Endian
    elf_bytes[16] = 2; // ET_EXEC
    elf_bytes[18] = ElfMachine.X86_64 as u8; // EM_X86_64 (62)
    elf_bytes[24] = 0x00; elf_bytes[25] = 0x10; elf_bytes[26] = 0x40; // entry = 0x401000

    if (hdr := ExeInspector.parseElf64Header(elf_bytes[0..64])) {
        // hdr.is_64bit => true
        // hdr.machine => 62 (ElfMachine.X86_64)
        // hdr.entry_point => 0x401000
    }
}
```

### 2. Mach-O 64 与 Windows PE/COFF 头部解析

```typescript
import {
    MachHeader64,
    MachoInspector,
    PeInspector,
} from "cap:elf";

fn demoMachoAndPe(): void {
    // 1. Mach-O 64-bit 帧头序列化与反序列化
    let macho_buf: [32]u8 = [0; 32];
    const mh = MachHeader64 {
        magic: MachoInspector.magic64(),
        cputype: MachoInspector.cpuTypeArm64(),
        cpusubtype: 0,
        filetype: MachoInspector.fileTypeExecute(),
        ncmds: 16,
        sizeofcmds: 1408,
        flags: 0x00200085,
        reserved: 0,
    };
    * = MachoInspector.encodeHeader64(macho_buf[0..32], mh);
    const parsed_mh = MachoInspector.parseHeader64(macho_buf[0..32]).?;
    // parsed_mh.cputype == MachoInspector.cpuTypeArm64()

    // 2. Windows PE/COFF 帧头解析
    let pe_buf: [96]u8 = [0; 96];
    pe_buf[0] = 'M'; pe_buf[1] = 'Z';
    pe_buf[60] = 64; // e_lfanew -> offset 64
    pe_buf[64] = 'P'; pe_buf[65] = 'E';
    pe_buf[68] = 0x64; pe_buf[69] = 0x86; // 0x8664 (AMD64)
    pe_buf[70] = 5; // 5 sections

    const coff = PeInspector.parseCoffHeader(pe_buf[0..96]).?;
    // coff.machine => 0x8664
    // coff.num_sections => 5
}
```

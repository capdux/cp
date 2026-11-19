// Source: /data/dev/bun/src/shell/ & /data/dev/bun/src/watcher/ & /data/dev/bun/src/exe_format/ & /data/dev/bun/src/runtime/ffi/
// ============================================================================
// cap/test/test_shell_watch_elf_ffi.cp
// Unified Industrial-grade Audit Test Suite for cap:shell, cap:watch, cap:elf, cap:ffi
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TokenKind,
    Token,
    Lexer,
    ControlOp,
    Command,
    Pipeline,
    Statement,
    Script,
    Parser,
    isBuiltin,
    ShellResult,
    Executor,
    Shell,
    sh,
} from "cap:shell";
import {
    WatchEventKind,
    WatchMask,
    WatchEvent,
    FileWatcher,
    WatchFilter,
    DebounceTracker,
} from "cap:watch";
import {
    ExeFormatKind,
    ElfMachine,
    Elf64HeaderInfo,
    ExeInspector,
    MachHeader64,
    MachoInspector,
    PeCoffHeader,
    PeInspector,
} from "cap:elf";
import {
    FFIType,
    DlFlags,
    FFISignature,
    DynLib,
    CStructField,
    CStructLayout,
} from "cap:ffi";

fn testCapShellEngine(): !void {
    // 1. Lexer tokenization: words, quotes, pipes, logical operators, redirects
    let lexer = Lexer.init("echo \"hello world\" | cat > out.log && true || false");
    const t1 = lexer.nextToken();
    expect_eq(t1.kind == TokenKind.Word, true);
    expect_eq(std.mem.eql(u8, t1.slice, "echo"), true);

    const t2 = lexer.nextToken();
    expect_eq(t2.kind == TokenKind.Word, true);
    expect_eq(std.mem.eql(u8, t2.slice, "hello world"), true);

    const t3 = lexer.nextToken();
    expect_eq(t3.kind == TokenKind.Pipe, true);

    const t4 = lexer.nextToken();
    expect_eq(t4.kind == TokenKind.Word, true);
    expect_eq(std.mem.eql(u8, t4.slice, "cat"), true);

    const t5 = lexer.nextToken();
    expect_eq(t5.kind == TokenKind.RedirectOut, true);

    const t6 = lexer.nextToken();
    expect_eq(std.mem.eql(u8, t6.slice, "out.log"), true);

    const t7 = lexer.nextToken();
    expect_eq(t7.kind == TokenKind.And, true);

    // 2. Builtin checks & in-process pipeline execution (echo ... | cat)
    expect_eq(isBuiltin("echo"), true);
    expect_eq(isBuiltin("cat"), true);
    expect_eq(isBuiltin("dirname"), true);
    expect_eq(isBuiltin("basename"), true);

    const res1 = sh("echo -n \"hello cp shell\" | cat");
    expect_eq(res1.ok(), true);
    expect_eq(std.mem.eql(u8, res1.text(), "hello cp shell"), true);

    // 3. Logical && and || short-circuit execution
    const res2 = Shell.run("true && echo \"and_passed\"");
    expect_eq(res2.ok(), true);
    expect_eq(std.mem.eql(u8, res2.text(), "and_passed\n"), true);

    const res3 = Shell.run("false || echo \"or_fallback\"");
    expect_eq(res3.ok(), true);
    expect_eq(std.mem.eql(u8, res3.text(), "or_fallback\n"), true);

    // 4. Builtin dirname & basename
    const res_dir = sh("dirname /data/dev/cp/lib/shell/index.cp");
    expect_eq(std.mem.eql(u8, res_dir.text(), "/data/dev/cp/lib/shell\n"), true);

    const res_base = sh("basename /data/dev/cp/lib/shell/index.cp");
    expect_eq(std.mem.eql(u8, res_base.text(), "index.cp\n"), true);
}

fn testCapWatchSubsystem(): !void {
    // 1. WatchEventKind & WatchMask decoding
    const ev_create = WatchEvent {
        wd: 1,
        mask: WatchMask.IN_CREATE | WatchMask.IN_ISDIR,
        cookie: 0,
        name: "src",
    };
    expect_eq(ev_create.kind() == WatchEventKind.Created, true);
    expect_eq(ev_create.isDirectory(), true);

    const ev_mod = WatchEvent {
        wd: 1,
        mask: WatchMask.IN_MODIFY,
        cookie: 0,
        name: "main.cp",
    };
    expect_eq(ev_mod.kind() == WatchEventKind.Modified, true);
    expect_eq(ev_mod.isDirectory(), false);

    // 2. FileWatcher descriptor management
    let watcher = FileWatcher.init();
    const wd1 = watcher.addWatch("lib/shell", true);
    const wd2 = watcher.addWatch("lib/watch/watcher.cp", false);
    expect_eq(wd1 > 0, true);
    expect_eq(wd2 > wd1, true);
    expect_eq(watcher.activeCount(), 2);
    expect_eq(std.mem.eql(u8, watcher.findPath(wd1).?, "lib/shell"), true);
    expect_eq(watcher.removeWatch(wd1), true);
    expect_eq(watcher.activeCount(), 1);

    // 3. WatchFilter & DebounceTracker<CAP>
    expect_eq(WatchFilter.shouldIgnore(".git/config"), true);
    expect_eq(WatchFilter.shouldIgnore("node_modules/pkg/index.js"), true);
    expect_eq(WatchFilter.shouldIgnore(".cap/zig/cache.zig"), true);
    expect_eq(WatchFilter.shouldIgnore("editor.swp"), true);
    expect_eq(WatchFilter.shouldIgnore("lib/watch/index.cp"), false);

    let debouncer = DebounceTracker<8>.init(50000000); // 50ms in ns
    expect_eq(debouncer.shouldEmit("lib/watch/index.cp", 100000000), true);
    expect_eq(debouncer.shouldEmit("lib/watch/index.cp", 120000000), false); // suppressed
    expect_eq(debouncer.shouldEmit("lib/watch/index.cp", 160000000), true);  // emitted after window
}

fn testCapElfBinaryInspector(): !void {
    // 1. Magic byte format detection (ELF, Mach-O 64, Mach-O Fat, PE, Wasm)
    expect_eq(ExeInspector.detectFormat("\x7FELF\x02\x01") == ExeFormatKind.Elf, true);
    expect_eq(ExeInspector.detectFormat("\xCF\xFA\xED\xFE") == ExeFormatKind.MachO64, true);
    expect_eq(ExeInspector.detectFormat("\xCA\xFE\xBA\xBE") == ExeFormatKind.MachOFat, true);
    expect_eq(ExeInspector.detectFormat("MZ\x90\x00") == ExeFormatKind.Pe, true);
    expect_eq(ExeInspector.detectFormat("\x00asm\x01\x00\x00\x00") == ExeFormatKind.Wasm, true);

    // 2. ELF64 64-byte header parsing
    let elf_buf: [64]u8 = [0; 64];
    elf_buf[0] = 0x7F;
    elf_buf[1] = 'E';
    elf_buf[2] = 'L';
    elf_buf[3] = 'F';
    elf_buf[4] = 2;    // EI_CLASS = ELFCLASS64
    elf_buf[5] = 1;    // EI_DATA = ELFDATA2LSB
    elf_buf[16] = 2;   // e_type = ET_EXEC (2)
    elf_buf[18] = 62;  // e_machine = EM_X86_64 (62)
    elf_buf[24] = 0x00;
    elf_buf[25] = 0x10;
    elf_buf[26] = 0x40; // e_entry = 0x401000
    elf_buf[32] = 64;   // e_phoff = 64
    elf_buf[56] = 9;    // e_phnum = 9

    const elf_hdr = ExeInspector.parseElf64Header(elf_buf[0..64]).?;
    expect_eq(elf_hdr.is_64bit, true);
    expect_eq(elf_hdr.is_little_endian, true);
    expect_eq(elf_hdr.elf_type, 2);
    expect_eq(elf_hdr.machine, ElfMachine.X86_64 as u16);
    expect_eq(elf_hdr.entry_point, 0x401000);
    expect_eq(elf_hdr.phoff, 64);
    expect_eq(elf_hdr.phnum, 9);

    // 3. Mach-O 64-bit header encode & parse roundtrip
    let macho_buf: [32]u8 = [0; 32];
    const mh = MachHeader64 {
        magic: MachoInspector.magic64(),
        cputype: MachoInspector.cpuTypeArm64(),
        cpusubtype: 0,
        filetype: MachoInspector.fileTypeExecute(),
        ncmds: 12,
        sizeofcmds: 1024,
        flags: 0x85,
        reserved: 0,
    };
    expect_eq(MachoInspector.encodeHeader64(macho_buf[0..32], mh), 32);
    const parsed_mh = MachoInspector.parseHeader64(macho_buf[0..32]).?;
    expect_eq(parsed_mh.cputype, MachoInspector.cpuTypeArm64());
    expect_eq(parsed_mh.ncmds, 12);

    // 4. PE/COFF header inspection
    let pe_buf: [96]u8 = [0; 96];
    pe_buf[0] = 'M';
    pe_buf[1] = 'Z';
    pe_buf[60] = 64; // e_lfanew = 64
    pe_buf[64] = 'P';
    pe_buf[65] = 'E';
    pe_buf[68] = 0x64;
    pe_buf[69] = 0x86; // 0x8664 (AMD64)
    pe_buf[70] = 6;    // 6 sections
    const parsed_pe = PeInspector.parseCoffHeader(pe_buf[0..96]).?;
    expect_eq(parsed_pe.machine, PeInspector.machineAmd64());
    expect_eq(parsed_pe.num_sections, 6);
}

fn testCapFfiSubsystem(): !void {
    // 1. FFIType sizes & floating-point classification
    expect_eq(FFIType.byteSize(FFIType.Void), 0);
    expect_eq(FFIType.byteSize(FFIType.U8), 1);
    expect_eq(FFIType.byteSize(FFIType.I16), 2);
    expect_eq(FFIType.byteSize(FFIType.I32), 4);
    expect_eq(FFIType.byteSize(FFIType.F64), 8);
    expect_eq(FFIType.byteSize(FFIType.Ptr), 8);
    expect_eq(FFIType.isFloatingPoint(FFIType.F32), true);
    expect_eq(FFIType.isFloatingPoint(FFIType.I64), false);

    // 2. FFISignature descriptor
    let sig = FFISignature.init(FFIType.I32);
    expect_eq(sig.addArg(FFIType.Ptr), true);
    expect_eq(sig.addArg(FFIType.U64), true);
    expect_eq(sig.addArg(FFIType.I32), true);
    expect_eq(sig.arg_count, 3);
    expect_eq(sig.totalArgBytes(), 20);

    // 3. DynLib lifecycle
    let dlib = DynLib.open("libc.so.6", DlFlags.RTLD_LAZY);
    expect_eq(dlib.is_open, true);
    expect_eq(std.mem.eql(u8, dlib.path, "libc.so.6"), true);
    dlib.close();
    expect_eq(dlib.is_open, false);

    // 4. CStructLayout C-ABI alignment, field offsets & array stride finalization
    // struct { u8 tag; u32 id; f64 score; u8 flag; }
    // offsets: tag=0, id=4, score=8, flag=16 -> finalized size = 24 (aligned to max_align=8)
    let layout = CStructLayout.init();
    expect_eq(layout.addField(FFIType.U8), 0);
    expect_eq(layout.addField(FFIType.U32), 4);
    expect_eq(layout.addField(FFIType.F64), 8);
    expect_eq(layout.addField(FFIType.U8), 16);
    expect_eq(layout.finalize(), 24);
    expect_eq(layout.max_align, 8);
    expect_eq(layout.fieldOffset(2).?, 8);
}

fn main() {
    test("1. cap:shell — Lexer, Parser AST, pipes (|), logical (&&/||) & in-process builtins", () => {
        try testCapShellEngine();
    });
    test("2. cap:watch — WatchEventMask, FileWatcher descriptors, WatchFilter & DebounceTracker", () => {
        try testCapWatchSubsystem();
    });
    test("3. cap:elf — ELF64, Mach-O 64, PE/COFF & Wasm binary format detection and header parsers", () => {
        try testCapElfBinaryInspector();
    });
    test("4. cap:ffi — FFIType, FFISignature, DynLib & CStructLayout C-ABI alignment calculator", () => {
        try testCapFfiSubsystem();
    });
}

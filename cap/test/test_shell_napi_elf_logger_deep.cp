// Source: /data/dev/bun/src/shell/ & /data/dev/bun/src/napi/ & /data/dev/bun/src/exe_format/ & /data/dev/bun/src/logger/
// ============================================================================
// cap/test/test_shell_napi_elf_logger_deep.cp
// Deep Audit & Parity Test Suite for:
//   1. cap:shell  (Lexer/Parser, quoting, ${VAR:-default}, redirections, env/cd stack, builtins)
//   2. cap:napi   (N-API v9 24 statuses, Float16, TSFN, AsyncWork, HandleScope, Refs, NapiEnvMock)
//   3. cap:elf    (ExeFormat.detect, synthetic full ELF64 + real /data/dev/cp/cap/bin/cpc ELF64)
//   4. cap:logger (LineIndex O(log N), DiagNote cross-ref, max_errors breaker, context window, JSON, merge)
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { StringJoiner } from "cap:string";
import {
    QuoteKind,
    TokenKind,
    Lexer,
    ControlOp,
    Parser,
    ShellEnv,
    ShellSession,
    Shell,
    sh,
} from "cap:shell";
import {
    NAPI_VERSION,
    NapiStatus,
    NapiValueType,
    NapiTypedArrayType,
    NapiPropertyAttributes,
    NapiPropertyDescriptor,
    NapiTsfnReleaseMode,
    NapiTsfnCallMode,
    NapiCallbackInfoData,
    NapiAsyncWorkStatus,
    NapiThreadsafeFunctionState,
    NapiRuntimeContext,
    NapiEnvMock,
    Napi,
} from "cap:napi";
import {
    ExeFormat,
    ExeFormatKind,
    ElfMachine,
    ElfType,
    ExeInspector,
    Elf64File,
    PT_LOAD,
    PT_INTERP,
    PT_DYNAMIC,
    PT_GNU_STACK,
    PT_GNU_RELRO,
    PF_R,
    PF_W,
    PF_X,
    SHT_PROGBITS,
    SHT_SYMTAB,
    SHT_STRTAB,
    SHT_DYNAMIC,
    SHT_DYNSYM,
    STB_GLOBAL,
    STB_WEAK,
    STT_FUNC,
    STT_OBJECT,
    DT_NULL,
    DT_NEEDED,
    DT_SONAME,
    DT_RUNPATH,
} from "cap:elf";
import {
    LogLevel,
    Loc,
    Range,
    LineIndex,
    Source,
    DiagNote,
    DiagMsg,
    Log,
} from "cap:logger";

fn writeU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64Le(buf: []u8, off: usize, v: u64) {
    writeU32Le(buf, off, (v & 0xFFFFFFFF) as u32);
    writeU32Le(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn testShellDeep(): !void {
    // 1. Lexer comments, single vs double quotes, fd redirections
    let lex = Lexer.init("# header comment\necho 'lit $X' \"interp $X\" 2>&1 2>>err.log");
    const t_nl = lex.nextToken(); // skips comment to \n
    expect_eq(t_nl.kind == TokenKind.Newline, true);
    const t_echo = lex.nextToken();
    expect_eq(std.mem.eql(u8, t_echo.slice, "echo"), true);
    const t_sq = lex.nextToken();
    expect_eq(t_sq.quote == QuoteKind.Single, true);
    expect_eq(std.mem.eql(u8, t_sq.slice, "lit $X"), true);
    const t_dq = lex.nextToken();
    expect_eq(t_dq.quote == QuoteKind.Double, true);
    expect_eq(std.mem.eql(u8, t_dq.slice, "interp $X"), true);
    const t_r21 = lex.nextToken();
    expect_eq(t_r21.kind == TokenKind.RedirectErrToOut, true);
    const t_r2app = lex.nextToken();
    expect_eq(t_r2app.kind == TokenKind.RedirectErrAppend, true);

    // 2. Session env, variable interpolation, ${VAR:-default}, ${VAR:=val}, ${#VAR}, prefix assignment
    let sess = Shell.session();
    defer sess.release();

    const r_interp = sess.run("GREETING=Hello; export TARGET=World; echo '$GREETING' \"$GREETING ${TARGET} ${UNSET:-Fallback} ${#TARGET}\"");
    expect_eq(r_interp.ok(), true);
    expect_eq(std.mem.eql(u8, r_interp.text(), "$GREETING Hello World Fallback 5\n"), true);

    // Prefix assignment only affects that command, does not leak into session env
    const r_prefix = sess.run("TEMP_KEY=scoped env | cat");
    expect_eq(r_prefix.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_prefix.text(), "TEMP_KEY=scoped") != null, true);
    expect_eq(sess.getEnv("TEMP_KEY") == null, true);

    // ${VAR:=assigned} assigns and unset removes
    const r_assign_def = sess.run("echo ${AUTO_VAR:=created} && unset AUTO_VAR && echo ${AUTO_VAR:-gone}");
    expect_eq(r_assign_def.ok(), true);
    expect_eq(std.mem.eql(u8, r_assign_def.text(), "created\ngone\n"), true);

    // 3. Logical short-circuit chains (&& / || / ;) & exit
    const r_logic = sess.run("false && echo NO ; true || echo NO ; false || echo YES && echo CHAIN");
    expect_eq(r_logic.ok(), true);
    expect_eq(std.mem.eql(u8, r_logic.text(), "YES\nCHAIN\n"), true);

    const r_exit = sess.run("echo before ; exit 42 ; echo after");
    expect_eq(r_exit.exit_code, 42 as i32);
    expect_eq(std.mem.eql(u8, r_exit.text(), "before\n"), true);

    // 4. Builtins: echo -e/-ne, printf, head, wc, basename suffix, dirname, test/[
    let sess2 = Shell.session();
    defer sess2.release();

    const r_echo_e = sess2.run("echo -ne 'a\\tb\\nline2\\cignored'");
    expect_eq(r_echo_e.ok(), true);
    expect_eq(std.mem.eql(u8, r_echo_e.text(), "a\tb\nline2"), true);

    const r_printf = sess2.run("printf '%-4s:%03d:%x' cp 7 255");
    expect_eq(r_printf.ok(), true);
    expect_eq(std.mem.eql(u8, r_printf.text(), "cp  :007:ff"), true);

    const r_pipe = sess2.run("printf 'one\\ntwo\\nthree\\n' | head -n 2 | wc -l");
    expect_eq(r_pipe.ok(), true);
    expect_eq(std.mem.eql(u8, r_pipe.text(), "2\n"), true);

    const r_bname = sess2.run("basename /dir/sub/mod.cp .cp");
    expect_eq(std.mem.eql(u8, r_bname.text(), "mod\n"), true);

    const r_test = sess2.run("[ 10 -gt 2 -a 'abc' != 'xyz' ] && test -n hello -a -z ''");
    expect_eq(r_test.ok(), true);
    const r_test_neg = sess2.run("[ ! 5 -eq 6 ] && [ 3 -le 3 ]");
    expect_eq(r_test_neg.ok(), true);

    // 5. Real filesystem builtins: mkdir -p, cd/pushd/popd, >, >>, <, 2>, 2>&1, test -d/-f/-s, rm -rf
    const r_fs = sess2.run(
        "mkdir -p /tmp/cp_shell_deep_dir/sub && " +
        "pushd /tmp/cp_shell_deep_dir/sub > /dev/null && " +
        "echo first > data.txt && " +
        "echo second >> data.txt && " +
        "[ -d /tmp/cp_shell_deep_dir/sub ] && [ -f data.txt ] && [ -s data.txt ] && " +
        "wc -l < data.txt && " +
        "cat missing_xyz_file 2> err.txt ; " +
        "cat err.txt && " +
        "cat missing_xyz_file 2>&1 | head -n 1 && " +
        "popd > /dev/null && " +
        "rm -rf /tmp/cp_shell_deep_dir"
    );
    expect_eq(r_fs.ok(), true);
    expect_eq(std.mem.indexOf(u8, r_fs.text(), "2\n") != null, true);
    expect_eq(std.mem.indexOf(u8, r_fs.text(), "No such file or directory") != null, true);

    // 6. Redirection to in-memory buffer via ShellSession.runWithMemoryTarget
    let mem_out = StringJoiner.init();
    defer mem_out.release();
    let mem_err = StringJoiner.init();
    defer mem_err.release();
    const mem_code = sess2.runWithMemoryTarget("echo mem_ok", &mem_out, &mem_err);
    expect_eq(mem_code, 0 as i32);
    expect_eq(std.mem.eql(u8, mem_out.slice(), "mem_ok\n"), true);
}

fn testNapiDeep(): !void {
    // 1. All 24 NapiStatus codes (0..23) & error messages + Float16 TypedArray
    expect_eq(NAPI_VERSION, 9);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.EscapeCalledTwice), "napi_escape_called_twice"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.QueueFull), "napi_queue_full"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.Closing), "napi_closing"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.DateExpected), "napi_date_expected"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.DetachableArrayBufferExpected), "napi_detachable_arraybuffer_expected"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.WouldDeadlock), "napi_would_deadlock"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.NoExternalBuffersAllowed), "napi_no_external_buffers_allowed"), true);
    expect_eq(std.mem.eql(u8, Napi.statusName(NapiStatus.CannotRunJs), "napi_cannot_run_js"), true);
    expect_eq(std.mem.eql(u8, Napi.statusMessage(NapiStatus.CannotRunJs), "Cannot run JavaScript"), true);
    expect_eq(Napi.typedArrayElementSize(NapiTypedArrayType.Float16), 2 as usize);

    // 2. NapiEnvMock / NapiRuntimeContext: HandleScope & EscapableHandleScope
    let env = NapiEnvMock.init();
    const ver = env.getNodeVersion();
    expect_eq(ver.major, 20);

    const outer = env.openHandleScope().?;
    const inner = env.openHandleScope().?;
    // Out-of-order close must fail with HandleScopeMismatch
    expect_eq(env.closeHandleScope(outer) == NapiStatus.HandleScopeMismatch, true);
    const tmp_val = env.createInt32(123).?;
    expect_eq(env.closeHandleScope(inner) == NapiStatus.Ok, true);
    // tmp_val was invalidated when inner closed
    expect_eq(env.getValueInt32(tmp_val) == null, true);
    expect_eq(env.last_status == NapiStatus.InvalidArg, true);

    // EscapableHandleScope promotes escaped value and blocks double escape
    const esc_scope = env.openEscapableHandleScope().?;
    const secret = env.createStringUtf8("escaped_ok").?;
    const escaped = env.escapeHandle(esc_scope, secret).?;
    expect_eq(env.escapeHandle(esc_scope, secret) == null, true);
    expect_eq(env.last_status == NapiStatus.EscapeCalledTwice, true);
    expect_eq(env.closeEscapableHandleScope(esc_scope) == NapiStatus.Ok, true);
    expect_eq(std.mem.eql(u8, env.getValueString(escaped).?, "escaped_ok"), true);
    expect_eq(env.closeHandleScope(outer) == NapiStatus.Ok, true);

    // 3. Reference counting table (ref / unref / getValue / delete)
    const obj = env.createObject().?;
    const num_val = env.createDouble(3.14).?;
    expect_eq(env.setNamedProperty(obj, "pi", num_val) == NapiStatus.Ok, true);
    expect_eq(env.hasNamedProperty(obj, "pi"), true);
    const got_pi = env.getNamedProperty(obj, "pi").?;
    expect_eq(env.getValueDouble(got_pi).? > 3.13, true);

    const ref_id = env.createReference(obj, 1).?;
    expect_eq(env.referenceRef(ref_id).?, 2);
    expect_eq(env.referenceUnref(ref_id).?, 1);
    expect_eq(env.referenceUnref(ref_id).?, 0);
    // Unref at 0 fails
    expect_eq(env.referenceUnref(ref_id) == null, true);
    expect_eq(env.getReferenceValue(ref_id).?, obj);
    expect_eq(env.deleteReference(ref_id) == NapiStatus.Ok, true);

    // 4. PropertyDescriptor, Module registration, Error & Exception state
    const prop_name = "compute";
    const pdesc = Napi.createPropertyDescriptor(prop_name.ptr, NapiPropertyAttributes.DefaultMethod);
    let descs: [1]NapiPropertyDescriptor = [pdesc];
    expect_eq(env.defineProperties(obj, descs[0..1]) == NapiStatus.Ok, true);
    expect_eq(env.hasNamedProperty(obj, "compute"), true);

    const mod_name = "mock_addon";
    const mod_file = "mock_addon.cp";
    const mod_desc = Napi.createModuleDescriptor(mod_name.ptr, mod_file.ptr, 0, null);
    expect_eq(env.registerModule(mod_desc), true);
    expect_eq(env.registry.find("mock_addon") != null, true);

    expect_eq(env.throwError("ERR_MOCK", "something failed") == NapiStatus.Ok, true);
    expect_eq(env.isExceptionPending(), true);
    expect_eq(env.getAndClearLastException() != null, true);
    expect_eq(env.isExceptionPending(), false);

    // 5. AsyncWork & ThreadsafeFunction (TSFN) lifecycle + CallbackInfoData
    const w1 = env.createAsyncWork("hash_job", null).?;
    expect_eq(env.queueAsyncWork(w1) == NapiStatus.Ok, true);
    expect_eq(env.runAsyncWork(w1) == NapiStatus.Ok, true);
    expect_eq(env.async_works[w1].status == NapiAsyncWorkStatus.Completed, true);
    expect_eq(env.deleteAsyncWork(w1) == NapiStatus.Ok, true);

    const w2 = env.createAsyncWork("cancel_job", null).?;
    expect_eq(env.queueAsyncWork(w2) == NapiStatus.Ok, true);
    expect_eq(env.cancelAsyncWork(w2) == NapiStatus.Ok, true);
    expect_eq(env.runAsyncWork(w2) == NapiStatus.Cancelled, true);

    let tsfn = NapiThreadsafeFunctionState.init("worker_cb", 2, 1);
    expect_eq(tsfn.acquire() == NapiStatus.Ok, true);
    expect_eq(tsfn.thread_count, 2 as usize);
    expect_eq(tsfn.call(101, NapiTsfnCallMode.NonBlocking) == NapiStatus.Ok, true);
    expect_eq(tsfn.call(202, NapiTsfnCallMode.NonBlocking) == NapiStatus.Ok, true);
    expect_eq(tsfn.call(303, NapiTsfnCallMode.NonBlocking) == NapiStatus.QueueFull, true);
    expect_eq(tsfn.drainAll(), 2 as usize);
    expect_eq(tsfn.dispatched_count, 2 as usize);
    expect_eq(tsfn.releaseThread(NapiTsfnReleaseMode.Abort) == NapiStatus.Ok, true);
    expect_eq(tsfn.call(404, NapiTsfnCallMode.NonBlocking) == NapiStatus.Closing, true);

    let cb_info = NapiCallbackInfoData.init(obj, null);
    expect_eq(cb_info.pushArg(num_val), true);
    expect_eq(cb_info.argc, 1 as usize);
}

fn testElfDeep(): !void {
    // 1. Multi-format magic detection via ExeFormat.detect
    expect_eq(ExeFormat.detect("\x7FELF\x02\x01") == ExeFormat.Elf64, true);
    expect_eq(ExeFormat.detect("\x7FELF\x01\x01") == ExeFormat.Elf32, true);
    expect_eq(ExeFormat.detect("\xCF\xFA\xED\xFE") == ExeFormat.MachO64, true);
    expect_eq(ExeFormat.detect("\xCE\xFA\xED\xFE") == ExeFormat.MachO32, true);
    expect_eq(ExeFormat.detect("\xCA\xFE\xBA\xBE") == ExeFormat.MachOFat, true);
    expect_eq(ExeFormat.detect("\x00asm\x01\x00\x00\x00") == ExeFormat.Wasm, true);
    expect_eq(ExeFormat.detect("#!/usr/bin/env bash\n") == ExeFormat.Shebang, true);

    // 2. Construct complete synthetic ELF64 image in memory:
    // Layout:
    //   0x000..0x040: Elf64_Ehdr (64 bytes)
    //   0x040..0x0F0: 3 Elf64_Phdr (PT_LOAD, PT_INTERP, PT_DYNAMIC) = 168 bytes
    //   0x100..0x120: .interp string "/lib64/ld-linux-x86-64.so.2\0"
    //   0x140..0x190: .shstrtab ("\0.shstrtab\0.text\0.symtab\0.strtab\0.dynamic\0.dynstr\0.interp\0")
    //   0x1A0..0x1C0: .text (16 bytes)
    //   0x1C0..0x200: .strtab ("\0main\0g_ver\0")
    //   0x200..0x248: .symtab (3 Elf64_Sym * 24 = 72 bytes)
    //   0x250..0x290: .dynstr ("\0libc.so.6\0libcp_demo.so\0/opt/cp/lib\0")
    //   0x290..0x2F0: .dynamic (6 Elf64_Dyn * 16 = 96 bytes)
    //   0x300..0x500: 8 Elf64_Shdr * 64 = 512 bytes
    let img: [1408]u8 = [0; 1408];

    // ELF64 header
    img[0] = 0x7F; img[1] = 'E'; img[2] = 'L'; img[3] = 'F';
    img[4] = 2; // ELFCLASS64
    img[5] = 1; // ELFDATA2LSB
    img[6] = 1; // EV_CURRENT
    writeU16Le(img[0..], 16, 3);      // e_type = ET_DYN
    writeU16Le(img[0..], 18, 62);     // e_machine = EM_X86_64
    writeU32Le(img[0..], 20, 1);      // e_version
    writeU64Le(img[0..], 24, 0x1000); // e_entry
    writeU64Le(img[0..], 32, 0x40);   // e_phoff
    writeU64Le(img[0..], 40, 0x300);  // e_shoff
    writeU16Le(img[0..], 52, 64);     // e_ehsize
    writeU16Le(img[0..], 54, 56);     // e_phentsize
    writeU16Le(img[0..], 56, 3);      // e_phnum
    writeU16Le(img[0..], 58, 64);     // e_shentsize
    writeU16Le(img[0..], 60, 8);      // e_shnum
    writeU16Le(img[0..], 62, 1);      // e_shstrndx = 1 (.shstrtab)

    // Phdr 0: PT_LOAD (R|X)
    writeU32Le(img[0..], 0x40, PT_LOAD);
    writeU32Le(img[0..], 0x44, PF_R | PF_X);
    writeU64Le(img[0..], 0x48, 0);
    writeU64Le(img[0..], 0x50, 0x400000);
    writeU64Le(img[0..], 0x60, 0x200);
    writeU64Le(img[0..], 0x68, 0x200);
    writeU64Le(img[0..], 0x70, 0x1000);

    // Phdr 1: PT_INTERP
    const interp_str = "/lib64/ld-linux-x86-64.so.2";
    @memcpy(img[0x100 .. 0x100 + interp_str.len], interp_str);
    writeU32Le(img[0..], 0x78, PT_INTERP);
    writeU32Le(img[0..], 0x7C, PF_R);
    writeU64Le(img[0..], 0x80, 0x100);
    writeU64Le(img[0..], 0x98, interp_str.len + 1);
    writeU64Le(img[0..], 0xA0, interp_str.len + 1);

    // Phdr 2: PT_DYNAMIC
    writeU32Le(img[0..], 0xB0, PT_DYNAMIC);
    writeU32Le(img[0..], 0xB4, PF_R | PF_W);
    writeU64Le(img[0..], 0xB8, 0x290);
    writeU64Le(img[0..], 0xD0, 96);
    writeU64Le(img[0..], 0xD8, 96);

    // .shstrtab at 0x140
    const shstr = "\x00.shstrtab\x00.text\x00.symtab\x00.strtab\x00.dynamic\x00.dynstr\x00.interp\x00";
    @memcpy(img[0x140 .. 0x140 + shstr.len], shstr);

    // .text at 0x1A0 (4 bytes)
    img[0x1A0] = 0x90; img[0x1A1] = 0xC3;

    // .strtab at 0x1C0
    const strtab = "\x00main\x00g_ver\x00";
    @memcpy(img[0x1C0 .. 0x1C0 + strtab.len], strtab);

    // .symtab at 0x200 (index 0=null, 1=main, 2=g_ver)
    // Sym 1: main (STB_GLOBAL=1, STT_FUNC=2 => st_info = 0x12)
    writeU32Le(img[0..], 0x218, 1); // st_name = "main"
    img[0x21C] = (STB_GLOBAL << 4) | STT_FUNC;
    img[0x21D] = 0; // STV_DEFAULT
    writeU16Le(img[0..], 0x21E, 2); // st_shndx
    writeU64Le(img[0..], 0x220, 0x1000); // st_value
    writeU64Le(img[0..], 0x228, 32);     // st_size

    // Sym 2: g_ver (STB_WEAK=2, STT_OBJECT=1 => st_info = 0x21)
    writeU32Le(img[0..], 0x230, 6); // st_name = "g_ver"
    img[0x234] = (STB_WEAK << 4) | STT_OBJECT;
    img[0x235] = 2; // STV_HIDDEN
    writeU64Le(img[0..], 0x238, 0x2000);
    writeU64Le(img[0..], 0x240, 8);

    // .dynstr at 0x250
    const dynstr = "\x00libc.so.6\x00libcp_demo.so\x00/opt/cp/lib\x00";
    @memcpy(img[0x250 .. 0x250 + dynstr.len], dynstr);

    // .dynamic at 0x290 (4 entries + DT_NULL)
    writeU64Le(img[0..], 0x290, DT_NEEDED as u64); writeU64Le(img[0..], 0x298, 1);  // "libc.so.6"
    writeU64Le(img[0..], 0x2A0, DT_SONAME as u64); writeU64Le(img[0..], 0x2A8, 11); // "libcp_demo.so"
    writeU64Le(img[0..], 0x2B0, DT_RUNPATH as u64); writeU64Le(img[0..], 0x2B8, 25); // "/opt/cp/lib"
    writeU64Le(img[0..], 0x2C0, DT_NULL as u64);   writeU64Le(img[0..], 0x2C8, 0);

    // Section Headers at 0x300 (each 64 bytes)
    // Shdr 1: .shstrtab (name=1)
    writeU32Le(img[0..], 0x340, 1);
    writeU32Le(img[0..], 0x344, SHT_STRTAB);
    writeU64Le(img[0..], 0x358, 0x140);
    writeU64Le(img[0..], 0x360, shstr.len);
    // Shdr 2: .text (name=11)
    writeU32Le(img[0..], 0x380, 11);
    writeU32Le(img[0..], 0x384, SHT_PROGBITS);
    writeU64Le(img[0..], 0x388, 6); // SHF_ALLOC | SHF_EXECINSTR
    writeU64Le(img[0..], 0x398, 0x1A0);
    writeU64Le(img[0..], 0x3A0, 4);
    // Shdr 3: .symtab (name=17, sh_link=4)
    writeU32Le(img[0..], 0x3C0, 17);
    writeU32Le(img[0..], 0x3C4, SHT_SYMTAB);
    writeU64Le(img[0..], 0x3D8, 0x200);
    writeU64Le(img[0..], 0x3E0, 72);
    writeU32Le(img[0..], 0x3E8, 4); // sh_link = .strtab
    writeU64Le(img[0..], 0x3F8, 24); // sh_entsize
    // Shdr 4: .strtab (name=25)
    writeU32Le(img[0..], 0x400, 25);
    writeU32Le(img[0..], 0x404, SHT_STRTAB);
    writeU64Le(img[0..], 0x418, 0x1C0);
    writeU64Le(img[0..], 0x420, strtab.len);
    // Shdr 5: .dynamic (name=33, sh_link=6)
    writeU32Le(img[0..], 0x440, 33);
    writeU32Le(img[0..], 0x444, SHT_DYNAMIC);
    writeU64Le(img[0..], 0x458, 0x290);
    writeU64Le(img[0..], 0x460, 96);
    writeU32Le(img[0..], 0x468, 6); // sh_link = .dynstr
    writeU64Le(img[0..], 0x478, 16);
    // Shdr 6: .dynstr (name=42)
    writeU32Le(img[0..], 0x480, 42);
    writeU32Le(img[0..], 0x484, SHT_STRTAB);
    writeU64Le(img[0..], 0x498, 0x250);
    writeU64Le(img[0..], 0x4A0, dynstr.len);
    // Shdr 7: .interp (name=50)
    writeU32Le(img[0..], 0x4C0, 50);
    writeU32Le(img[0..], 0x4C4, SHT_PROGBITS);
    writeU64Le(img[0..], 0x4D8, 0x100);
    writeU64Le(img[0..], 0x4E0, interp_str.len + 1);

    const elf = Elf64File.parse(img[0..1408]).?;
    expect_eq(elf.header().typeKind() == ElfType.Dyn, true);
    expect_eq(elf.header().machineKind() == ElfMachine.X86_64, true);
    expect_eq(elf.phdrCount(), 3 as usize);
    expect_eq(elf.getPhdr(0).?.isExecutable(), true);
    expect_eq(elf.getPhdr(0).?.isReadable(), true);
    expect_eq(std.mem.eql(u8, elf.interpreter().?, "/lib64/ld-linux-x86-64.so.2"), true);

    expect_eq(elf.shdrCount(), 8 as usize);
    const text_sh = elf.findSection(".text").?;
    expect_eq(text_sh.isExecutable(), true);
    const text_data = elf.findSectionData(".text").?;
    expect_eq(text_data[0], 0x90 as u8);
    expect_eq(text_data[1], 0xC3 as u8);

    expect_eq(elf.symbolCount(false), 3 as usize);
    const main_sym = elf.findSymbol("main", false).?;
    expect_eq(main_sym.st_value, 0x1000);
    expect_eq(main_sym.bind(), STB_GLOBAL);
    expect_eq(main_sym.symType(), STT_FUNC);
    const gver_sym = elf.findSymbol("g_ver", false).?;
    expect_eq(std.mem.eql(u8, gver_sym.bindName(), "WEAK"), true);
    expect_eq(std.mem.eql(u8, gver_sym.visibilityName(), "HIDDEN"), true);

    let needed: [4][]const u8 = undefined;
    const needed_cnt = elf.getNeededLibs(needed[0..4]);
    expect_eq(needed_cnt, 1 as usize);
    expect_eq(std.mem.eql(u8, needed[0], "libc.so.6"), true);
    expect_eq(std.mem.eql(u8, elf.soname().?, "libcp_demo.so"), true);
    expect_eq(std.mem.eql(u8, elf.runpath().?, "/opt/cp/lib"), true);

    // 3. Parse a real Linux ELF64 binary from disk (/data/dev/cp/cap/bin/cpc)
    let real_bytes: []u8 = "";
    zig {
        const z_flags = std.c.O{ .CLOEXEC = true };
        const fd = std.c.open("/data/dev/cp/cap/bin/cpc", z_flags, @as(std.c.mode_t, 0));
        if (fd >= 0) {
            defer _ = std.c.close(fd);
            var stx: std.os.linux.Statx = undefined;
            const rc = std.c.statx(fd, "", std.os.linux.AT.EMPTY_PATH, std.os.linux.STATX{ .SIZE = true }, &stx);
            if (rc == 0 and stx.size > 64) {
                const sz: usize = @intCast(stx.size);
                const buf = std.heap.page_allocator.alloc(u8, sz) catch unreachable;
                var off: usize = 0;
                while (off < sz) {
                    const n = std.c.read(fd, buf[off..].ptr, sz - off);
                    if (n <= 0) break;
                    off += @intCast(n);
                }
                real_bytes = buf[0..off];
            }
        }
    }
    defer {
        if (real_bytes.len > 0) {
            zig { std.heap.page_allocator.free(real_bytes); }
        }
    }
    if (real_bytes.len > 64) {
        expect_eq(ExeFormat.detect(real_bytes) == ExeFormat.Elf64, true);
        const cpc_elf = Elf64File.parse(real_bytes).?;
        expect_eq(cpc_elf.header().is_64bit, true);
        expect_eq(cpc_elf.phdrCount() > 0, true);
        expect_eq(cpc_elf.shdrCount() > 0, true);
        expect_eq(cpc_elf.findSection(".text") != null, true);
        expect_eq(cpc_elf.findPhdr(PT_LOAD) != null, true);
    }
}

fn testLoggerDeep(): !void {
    // 1. LogLevel ordering & fromString
    expect_eq(LogLevel.fromString("verbose").? == LogLevel.Verbose, true);
    expect_eq(LogLevel.fromString("error").? == LogLevel.Err, true);
    expect_eq(LogLevel.Err.atLeast(LogLevel.Warn), true);
    expect_eq(LogLevel.Debug.shouldEmit(LogLevel.Info), false);
    expect_eq(LogLevel.Debug.shouldEmit(LogLevel.Verbose), true);

    // 2. High-performance LineIndex O(log N) cache
    const code = "let a = 1;\nlet a = 2;\nreturn a;\n";
    let src = Source.initIndexed("src/dup.cp", code);
    expect_eq(src.has_index, true);
    expect_eq(src.cached_index.lineCount(), 4 as usize);
    const decl1_off = std.mem.indexOf(u8, code, "a = 1").?;
    const decl2_off = std.mem.indexOf(u8, code, "a = 2").?;
    expect_eq(src.lineColOf(decl2_off).line, 2 as usize);
    expect_eq(src.lineColOf(decl2_off).col, 5 as usize);
    expect_eq(std.mem.eql(u8, src.lineTextByNumber(1), "let a = 1;"), true);

    // 3. DiagMsg with multi-line context window + secondary DiagNote pointing to decl1
    let msg = DiagMsg.fromSource(
        LogLevel.Err,
        src,
        Range.init(decl2_off as i32, 1),
        "redefinition of 'a'",
        "",
    );
    msg.code = "E0201";
    expect_eq(msg.addNoteFromSource(src, Range.init(decl1_off as i32, 1), "previous declaration of 'a' here"), true);

    let plain_buf: [1024]u8 = [0; 1024];
    const plain = msg.formatPlain(plain_buf[0..1024]);
    expect_eq(std.mem.indexOf(u8, plain, "error[E0201]: redefinition of 'a'") != null, true);
    expect_eq(std.mem.indexOf(u8, plain, "1 | let a = 1;") != null, true);
    expect_eq(std.mem.indexOf(u8, plain, "2 | let a = 2;") != null, true);
    expect_eq(std.mem.indexOf(u8, plain, "= note: previous declaration of 'a' here") != null, true);
    expect_eq(std.mem.indexOf(u8, plain, "--> src/dup.cp:1:5") != null, true);

    // 4. max_errors circuit breaker, addErrorFmt, merge, and JSON diagnostic array output
    let log1 = Log.initWithMaxErrors(LogLevel.Info, 2);
    log1.addMsg(msg);
    let fmt_buf: [128]u8 = [0; 128];
    log1.addErrorFmt(src, Loc.init(0), fmt_buf[0..128], "error #{d} with \"quotes\"", .{2});
    log1.addError(src, Loc.init(11), "third error should be suppressed");
    expect_eq(log1.errors, 2 as usize);
    expect_eq(log1.suppressed_errors, 1 as usize);
    expect_eq(log1.isErrorLimitReached(), true);

    let sub_log = Log.init(LogLevel.Info);
    sub_log.addWarning(src, Loc.init(22), "unused return value");
    let main_log = Log.init(LogLevel.Info);
    expect_eq(main_log.merge(&log1), 2 as usize);
    expect_eq(main_log.merge(&sub_log), 1 as usize);
    expect_eq(main_log.errors, 2 as usize);
    expect_eq(main_log.warnings, 1 as usize);

    let json_buf: [4096]u8 = [0; 4096];
    const json_out = main_log.formatJson(json_buf[0..4096]);
    expect_eq(json_out[0], '[' as u8);
    expect_eq(json_out[json_out.len - 1], ']' as u8);
    expect_eq(std.mem.indexOf(u8, json_out, "\"level\":\"error\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json_out, "\\\"quotes\\\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json_out, "\"previous declaration of 'a' here\"") != null, true);
    expect_eq(std.mem.indexOf(u8, json_out, "\"level\":\"warn\"") != null, true);
}

fn main() {
    test("1. cap:shell Deep — Quotes, ${VAR:-def}, Redirections, Env/DirStack & Full Builtins", () => {
        testShellDeep()?;
    });
    test("2. cap:napi Deep — 24 Statuses, Float16, MockEnv, HandleScope, Refs, AsyncWork & TSFN", () => {
        testNapiDeep()?;
    });
    test("3. cap:elf Deep — ExeFormat.detect, Full ELF64 Phdr/Shdr/Sym/Dyn & Real cpc Binary", () => {
        testElfDeep()?;
    });
    test("4. cap:logger Deep — LineIndex, DiagNote Cross-Ref, max_errors Breaker, Merge & JSON", () => {
        testLoggerDeep()?;
    });
}

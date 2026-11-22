import { test, expect } from "cap:test";
import {
    EDITOR_DEFAULT_PREFERENCE_COUNT,
    EditorKind,
    OpenerPlatform,
    buildOpenEditorCommand,
    buildOpenUrlCommand,
    defaultEditorPreferenceAt,
    normalizeEditorTempFilename,
    platformOpener,
    resolveEditorFromPreference,
} from "cap:cli";

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("cli open: EditorKind.byName aliases, args stripping, paths, and binName", () => {
    expect(EditorKind.byName("code") == EditorKind.VsCode).toBe(true);
    expect(EditorKind.byName("vscode") == EditorKind.VsCode).toBe(true);
    expect(EditorKind.byName("code --wait") == EditorKind.VsCode).toBe(true);
    expect(EditorKind.byName("/usr/local/bin/subl -w") == EditorKind.Sublime).toBe(true);
    expect(EditorKind.byName("sublime") == EditorKind.Sublime).toBe(true);
    expect(EditorKind.byName("mate") == EditorKind.TextMate).toBe(true);
    expect(EditorKind.byName("textmate") == EditorKind.TextMate).toBe(true);
    expect(EditorKind.byName("atom") == EditorKind.Atom).toBe(true);
    expect(EditorKind.byName("idea") == EditorKind.IntelliJ).toBe(true);
    expect(EditorKind.byName("webstorm") == EditorKind.WebStorm).toBe(true);
    expect(EditorKind.byName("nvim") == EditorKind.Neovim).toBe(true);
    expect(EditorKind.byName("neovim") == EditorKind.Neovim).toBe(true);
    expect(EditorKind.byName("/usr/bin/vi") == EditorKind.Vim).toBe(true);
    expect(EditorKind.byName("vim") == EditorKind.Vim).toBe(true);
    expect(EditorKind.byName("emacs") == EditorKind.Emacs).toBe(true);
    expect(EditorKind.byName("unknown-editor") == null).toBe(true);
    expect(EditorKind.byName("") == null).toBe(true);

    expect(sliceEq(EditorKind.VsCode.binName(), "code")).toBe(true);
    expect(sliceEq(EditorKind.Sublime.binName(), "subl")).toBe(true);
    expect(sliceEq(EditorKind.Neovim.binName(), "nvim")).toBe(true);
    expect(sliceEq(EditorKind.IntelliJ.binName(), "idea")).toBe(true);
    expect(EditorKind.IntelliJ.isJetBrains()).toBe(true);
    expect(EditorKind.WebStorm.isJetBrains()).toBe(true);
    expect(EditorKind.VsCode.isJetBrains()).toBe(false);
    expect(EditorKind.Vim.isTerminalEditor()).toBe(true);
    expect(EditorKind.Neovim.isTerminalEditor()).toBe(true);
    expect(EditorKind.Emacs.isTerminalEditor()).toBe(true);
    expect(EditorKind.Sublime.isTerminalEditor()).toBe(false);
});

test("cli open: env detection, preference resolution, default order and macOS fallback paths", () => {
    // EDITOR takes precedence over VISUAL
    expect(EditorKind.detectFromEnv("nvim", "code") == EditorKind.Neovim).toBe(true);
    expect(EditorKind.detectFromEnv("unknown", "code") == EditorKind.VsCode).toBe(true);
    expect(EditorKind.detectFromEnv("", "") == null).toBe(true);

    // Explicit absolute custom editor path
    const custom = resolveEditorFromPreference("/opt/helix/hx", "code", "vim");
    expect(custom.kind == EditorKind.Other).toBe(true);
    expect(custom.is_absolute_override).toBe(true);
    expect(sliceEq(custom.binary, "/opt/helix/hx")).toBe(true);

    // Explicit absolute known editor path
    const abs_vim = resolveEditorFromPreference("/usr/bin/vim", "code", "");
    expect(abs_vim.kind == EditorKind.Vim).toBe(true);
    expect(abs_vim.is_absolute_override).toBe(true);
    expect(sliceEq(abs_vim.binary, "/usr/bin/vim")).toBe(true);

    // Relative preference overrides env
    const pref = resolveEditorFromPreference("subl", "vim", "code");
    expect(pref.kind == EditorKind.Sublime).toBe(true);
    expect(sliceEq(pref.binary, "subl")).toBe(true);

    // Env fallback when preference is empty
    const env_res = resolveEditorFromPreference("", "", "vscode");
    expect(env_res.kind == EditorKind.VsCode).toBe(true);
    expect(sliceEq(env_res.binary, "code")).toBe(true);

    // Default preference list order and macOS fallback paths
    expect(EDITOR_DEFAULT_PREFERENCE_COUNT).toBe(8);
    expect(defaultEditorPreferenceAt(0) == EditorKind.VsCode).toBe(true);
    expect(defaultEditorPreferenceAt(1) == EditorKind.Sublime).toBe(true);
    expect(defaultEditorPreferenceAt(7) == EditorKind.Vim).toBe(true);
    expect(defaultEditorPreferenceAt(8) == null).toBe(true);

    expect(EditorKind.VsCode.macFallbackPathCount()).toBe(2);
    expect(EditorKind.Sublime.macFallbackPathCount()).toBe(4);
    expect(EditorKind.Atom.macFallbackPathCount()).toBe(1);
    expect(EditorKind.Vim.macFallbackPathCount()).toBe(0);
    if (p0 := EditorKind.VsCode.macFallbackPathAt(0)) {
        expect(sliceEq(p0, "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code")).toBe(true);
    } else {
        expect(false).toBe(true);
    }
});

test("cli open: buildOpenEditorCommand for VS Code, Sublime, JetBrains, and TextMate", () => {
    let scratch: [512]u8 = [0; 512];

    // VS Code with line & column -> --goto file:line:col
    const vs = buildOpenEditorCommand(
        EditorKind.VsCode,
        "code",
        "/src/app.cp",
        "42",
        "7",
        OpenerPlatform.Linux,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(vs.argc).toBe(3);
    expect(sliceEq(vs.args[0], "code")).toBe(true);
    expect(sliceEq(vs.args[1], "--goto")).toBe(true);
    expect(sliceEq(vs.args[2], "/src/app.cp:42:7")).toBe(true);

    // VS Code without line -> omits --goto
    const vs_no_line = buildOpenEditorCommand(
        EditorKind.VsCode,
        "code",
        "/src/app.cp",
        "",
        "",
        OpenerPlatform.Linux,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(vs_no_line.argc).toBe(2);
    expect(sliceEq(vs_no_line.args[1], "/src/app.cp")).toBe(true);

    // JetBrains (WebStorm / IntelliJ) omits column even if provided
    const jb = buildOpenEditorCommand(
        EditorKind.WebStorm,
        "webstorm",
        "/src/app.cp",
        "10",
        "5",
        OpenerPlatform.MacOS,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(jb.argc).toBe(2);
    expect(sliceEq(jb.args[0], "webstorm")).toBe(true);
    expect(sliceEq(jb.args[1], "/src/app.cp:10")).toBe(true);

    // TextMate uses --line line:col file
    const tm = buildOpenEditorCommand(
        EditorKind.TextMate,
        "mate",
        "/src/app.cp",
        "12",
        "3",
        OpenerPlatform.MacOS,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(tm.argc).toBe(4);
    expect(sliceEq(tm.args[0], "mate")).toBe(true);
    expect(sliceEq(tm.args[1], "--line")).toBe(true);
    expect(sliceEq(tm.args[2], "12:3")).toBe(true);
    expect(sliceEq(tm.args[3], "/src/app.cp")).toBe(true);
});

test("cli open: terminal editors wrap platform opener and macOS --args", () => {
    let scratch: [512]u8 = [0; 512];

    const mac_nvim = buildOpenEditorCommand(
        EditorKind.Neovim,
        "nvim",
        "/src/main.cp",
        "10",
        "1",
        OpenerPlatform.MacOS,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(mac_nvim.argc).toBe(5);
    expect(sliceEq(mac_nvim.args[0], "/usr/bin/open")).toBe(true);
    expect(sliceEq(mac_nvim.args[1], "nvim")).toBe(true);
    expect(sliceEq(mac_nvim.args[2], "--args")).toBe(true);
    expect(sliceEq(mac_nvim.args[3], "nvim")).toBe(true);
    expect(sliceEq(mac_nvim.args[4], "/src/main.cp")).toBe(true);

    const linux_vim = buildOpenEditorCommand(
        EditorKind.Vim,
        "vim",
        "/src/main.cp",
        "",
        "",
        OpenerPlatform.Linux,
        scratch[0..],
    ) catch {
        expect(false).toBe(true);
        return;
    };
    expect(linux_vim.argc).toBe(4);
    expect(sliceEq(linux_vim.args[0], "xdg-open")).toBe(true);
    expect(sliceEq(linux_vim.args[1], "vim")).toBe(true);
    expect(sliceEq(linux_vim.args[2], "vim")).toBe(true);
    expect(sliceEq(linux_vim.args[3], "/src/main.cp")).toBe(true);
});

test("cli open: cross-platform URL opener commands and WASI fallback", () => {
    expect(sliceEq(platformOpener(OpenerPlatform.MacOS), "/usr/bin/open")).toBe(true);
    expect(sliceEq(platformOpener(OpenerPlatform.Windows), "start")).toBe(true);
    expect(sliceEq(platformOpener(OpenerPlatform.Linux), "xdg-open")).toBe(true);

    const mac = buildOpenUrlCommand("https://cp-lang.org", OpenerPlatform.MacOS) catch {
        expect(false).toBe(true);
        return;
    };
    expect(mac.use_fallback).toBe(false);
    expect(mac.argc).toBe(2);
    expect(sliceEq(mac.args[0], "/usr/bin/open")).toBe(true);
    expect(sliceEq(mac.args[1], "https://cp-lang.org")).toBe(true);

    const android = buildOpenUrlCommand("https://cp-lang.org", OpenerPlatform.Android) catch {
        expect(false).toBe(true);
        return;
    };
    expect(android.argc).toBe(6);
    expect(sliceEq(android.args[0], "/system/bin/am")).toBe(true);
    expect(sliceEq(android.args[3], "android.intent.action.VIEW")).toBe(true);
    expect(sliceEq(android.args[5], "https://cp-lang.org")).toBe(true);

    const wasi = buildOpenUrlCommand("https://cp-lang.org", OpenerPlatform.Wasi) catch {
        expect(false).toBe(true);
        return;
    };
    expect(wasi.use_fallback).toBe(true);
    expect(wasi.argc).toBe(0);
});

test("cli open: temp filename .bun -> .bun.js normalization and error guards", () => {
    let out: [128]u8 = [0; 128];
    const n1 = normalizeEditorTempFilename("/tmp/cache/bundle.bun", out[0..]) catch 0;
    expect(sliceEq(out[0..n1], "bundle.bun.js")).toBe(true);

    const n2 = normalizeEditorTempFilename("/tmp/cache/index.ts", out[0..]) catch 0;
    expect(sliceEq(out[0..n2], "index.ts")).toBe(true);

    // Error guards: EmptyUrl, EmptyBinary, EmptyFile, BufferTooSmall
    let url_err = false;
    * = buildOpenUrlCommand("", OpenerPlatform.Linux) catch {
        url_err = true;
    };
    expect(url_err).toBe(true);

    let bin_err = false;
    * = buildOpenEditorCommand(EditorKind.VsCode, "", "/a.cp", "1", "1", OpenerPlatform.Linux, out[0..]) catch {
        bin_err = true;
    };
    expect(bin_err).toBe(true);

    let file_err = false;
    * = buildOpenEditorCommand(EditorKind.VsCode, "code", "", "1", "1", OpenerPlatform.Linux, out[0..]) catch {
        file_err = true;
    };
    expect(file_err).toBe(true);

    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = buildOpenEditorCommand(EditorKind.VsCode, "code", "/long/path/file.cp", "10", "2", OpenerPlatform.Linux, tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);
});

// Source: Absorbed from /data/dev/bun/src/cli/open.cp | Synced: 2026-11-23T20:20:17Z
// Pure-CP zero-allocation editor detection (`EDITOR`/`VISUAL`/user preference),
// macOS `.app` fallback path table, cross-platform URL opener command builder,
// and editor-specific `file:line:column` argv generator (`Bun.openInEditor` core).

/// 单次打开编辑器命令的最大 argv 参数数量
pub const EDITOR_MAX_ARGS: usize = 10;

/// 默认编辑器探测优先级列表长度
pub const EDITOR_DEFAULT_PREFERENCE_COUNT: usize = 8;

/// 编辑器与 URL 打开命令构建错误集
pub err EditorOpenError {
    BufferTooSmall,
    EmptyFile,
    EmptyBinary,
    EmptyUrl,
}

fn bytesEql(a: []const u8, b: []const u8): bool {
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

fn endsWith(s: []const u8, suffix: []const u8): bool {
    if (suffix.len > s.len) {
        return false;
    }
    return bytesEql(s[s.len - suffix.len..], suffix);
}

fn indexOfChar(s: []const u8, ch: u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ch) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn pathBasename(path: []const u8): []const u8 {
    let end = path.len;
    while (end > 0 && (path[end - 1] == '/' || path[end - 1] == '\\')) {
        end -= 1;
    }
    let i = end;
    while (i > 0) {
        i -= 1;
        if (path[i] == '/' || path[i] == '\\') {
            return path[i + 1..end];
        }
    }
    return path[0..end];
}

fn isAbsolutePath(path: []const u8): bool {
    if (path.len == 0) {
        return false;
    }
    if (path[0] == '/' || path[0] == '\\') {
        return true;
    }
    if (path.len >= 3 && path[1] == ':' && (path[2] == '/' || path[2] == '\\')) {
        const c = path[0];
        return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
    }
    return false;
}

fn appendBytes(out: []u8, pos: usize, src: []const u8): usize! {
    if (src.len > out.len - pos) {
        return err.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < src.len) {
        out[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// 宿主操作系统平台类别（决定默认 URL/应用打开器与 `--args` 标志）
pub enum OpenerPlatform: u8 {
    MacOS = 0,
    Linux = 1,
    Windows = 2,
    Android = 3,
    Wasi = 4,
}

/// 返回指定平台的系统默认打开器命令路径
pub fn platformOpener(platform: OpenerPlatform): []const u8 {
    return switch (platform) {
        .MacOS => "/usr/bin/open",
        .Windows => "start",
        .Linux => "xdg-open",
        .Android => "/system/bin/am",
        .Wasi => "",
    };
}

/// 跨平台 URL 打开命令结构（零分配定长参数槽）
pub struct UrlOpenCommand {
    args: [6][]const u8 = [""; 6],
    argc: usize = 0,
    use_fallback: bool = false,
}

/// 构造跨平台 URL 打开命令（WASI 自动回退为打印 `-> <url>`）
pub fn buildOpenUrlCommand(url: []const u8, platform: OpenerPlatform): UrlOpenCommand! {
    if (url.len == 0) {
        return err.EmptyUrl;
    }
    if (platform == .Wasi) {
        return UrlOpenCommand{
            args: [""; 6],
            argc: 0,
            use_fallback: true,
        };
    }
    if (platform == .Android) {
        let cmd = UrlOpenCommand{
            args: [
                "/system/bin/am",
                "start",
                "-a",
                "android.intent.action.VIEW",
                "-d",
                url,
            ],
            argc: 6,
            use_fallback: false,
        };
        return cmd;
    }
    let cmd = UrlOpenCommand{
        args: [platformOpener(platform), url, "", "", "", ""],
        argc: 2,
        use_fallback: false,
    };
    return cmd;
}

/// 支持的代码编辑器枚举（严格对齐 Bun `src/cli/open.cp`）
pub enum EditorKind: u8 {
    None = 0,
    Sublime = 1,
    VsCode = 2,
    Atom = 3,
    TextMate = 4,
    IntelliJ = 5,
    WebStorm = 6,
    Vim = 7,
    Neovim = 8,
    Emacs = 9,
    Other = 10,

    /// 按编辑器名、可执行别名或完整路径识别 `EditorKind`（如 `"code --wait"` / `"/usr/bin/nvim"`）
    static fn byName(raw_name: []const u8): EditorKind? {
        if (raw_name.len == 0) {
            return null;
        }
        let first_word = raw_name;
        if (sp := indexOfChar(raw_name, ' ')) {
            first_word = raw_name[0..sp];
        }
        const base = pathBasename(first_word);
        if (bytesEql(base, "sublime") || bytesEql(base, "subl")) return .Sublime;
        if (bytesEql(base, "vscode") || bytesEql(base, "code")) return .VsCode;
        if (bytesEql(base, "textmate") || bytesEql(base, "mate")) return .TextMate;
        if (bytesEql(base, "atom")) return .Atom;
        if (bytesEql(base, "idea")) return .IntelliJ;
        if (bytesEql(base, "webstorm")) return .WebStorm;
        if (bytesEql(base, "nvim") || bytesEql(base, "neovim")) return .Neovim;
        if (bytesEql(base, "vim") || bytesEql(base, "vi")) return .Vim;
        if (bytesEql(base, "emacs")) return .Emacs;
        return null;
    }

    /// 依次从 `EDITOR` 与 `VISUAL` 环境变量值中探测编辑器（对齐 Bun `Editor.detect`）
    static fn detectFromEnv(editor_env: []const u8, visual_env: []const u8): EditorKind? {
        if (editor_env.len > 0) {
            if (ed := EditorKind.byName(editor_env)) {
                return ed;
            }
        }
        if (visual_env.len > 0) {
            if (ed := EditorKind.byName(visual_env)) {
                return ed;
            }
        }
        return null;
    }

    /// 返回标准可执行文件名（对应 Bun `Editor.bin_name`）
    fn binName(): []const u8 {
        return switch (_) {
            .Sublime => "subl",
            .VsCode => "code",
            .Atom => "atom",
            .TextMate => "mate",
            .IntelliJ => "idea",
            .WebStorm => "webstorm",
            .Vim => "vim",
            .Neovim => "nvim",
            .Emacs => "emacs",
            .None, .Other => "",
        };
    }

    /// 是否为 JetBrains IDE（`file:line` 跳转，不附加 `:column`）
    fn isJetBrains(): bool {
        return _ == .IntelliJ || _ == .WebStorm;
    }

    /// 是否为终端编辑器（`vim` / `neovim` / `emacs`）
    fn isTerminalEditor(): bool {
        return _ == .Vim || _ == .Neovim || _ == .Emacs;
    }

    /// 返回 macOS 常见 `.app` 捆绑包内的回退二进制路径数量
    fn macFallbackPathCount(): usize {
        return switch (_) {
            .VsCode => 2,
            .Atom => 1,
            .Sublime => 4,
            else => 0,
        };
    }

    /// 按索引返回 macOS 常见 `.app` 回退二进制路径（对应 Bun `Editor.bin_path`）
    fn macFallbackPathAt(idx: usize): ([]const u8)? {
        if (_ == .VsCode) {
            if (idx == 0) return "/Applications/Visual Studio Code.app/Contents/Resources/app/bin/code";
            if (idx == 1) return "/Applications/VSCodium.app/Contents/Resources/app/bin/code";
            return null;
        }
        if (_ == .Atom) {
            if (idx == 0) return "/Applications/Atom.app/Contents/Resources/app/atom.sh";
            return null;
        }
        if (_ == .Sublime) {
            if (idx == 0) return "/Applications/Sublime Text 4.app/Contents/SharedSupport/bin/subl";
            if (idx == 1) return "/Applications/Sublime Text 3.app/Contents/SharedSupport/bin/subl";
            if (idx == 2) return "/Applications/Sublime Text 2.app/Contents/SharedSupport/bin/subl";
            if (idx == 3) return "/Applications/Sublime Text.app/Contents/SharedSupport/bin/subl";
            return null;
        }
        return null;
    }
}

/// 按默认探测优先级索引返回编辑器类型（对齐 Bun `Editor.default_preference_list`）
pub fn defaultEditorPreferenceAt(idx: usize): EditorKind? {
    return switch (idx) {
        0 => .VsCode,
        1 => .Sublime,
        2 => .Atom,
        3 => .Neovim,
        4 => .WebStorm,
        5 => .IntelliJ,
        6 => .TextMate,
        7 => .Vim,
        else => null,
    };
}

/// 编辑器解析结果（纯逻辑决策，不触发文件系统 I/O）
pub struct EditorResolution {
    kind: EditorKind = .None,
    binary: []const u8 = "",
    is_absolute_override: bool = false,
}

/// 按「用户显式偏好（如 `bunfig.toml` `debug.editor`）→ `EDITOR` → `VISUAL`」顺序解析目标编辑器
pub fn resolveEditorFromPreference(
    preferred_name: []const u8,
    editor_env: []const u8,
    visual_env: []const u8,
): EditorResolution {
    if (preferred_name.len > 0) {
        if (isAbsolutePath(preferred_name)) {
            const kind = EditorKind.byName(preferred_name) ?? EditorKind.Other;
            return EditorResolution{
                kind: kind,
                binary: preferred_name,
                is_absolute_override: true,
            };
        }
        if (kind := EditorKind.byName(preferred_name)) {
            return EditorResolution{
                kind: kind,
                binary: kind.binName(),
                is_absolute_override: false,
            };
        }
    }

    if (kind := EditorKind.detectFromEnv(editor_env, visual_env)) {
        return EditorResolution{
            kind: kind,
            binary: kind.binName(),
            is_absolute_override: false,
        };
    }

    return EditorResolution{
        kind: .None,
        binary: "",
        is_absolute_override: false,
    };
}

/// 编辑器打开命令结构（零分配定长 argv 视图，切片指向输入参数或调用方 `scratch` 缓冲）
pub struct EditorOpenCommand {
    args: [EDITOR_MAX_ARGS][]const u8 = [""; EDITOR_MAX_ARGS],
    argc: usize = 0,
    scratch_used: usize = 0,
}

/// 构造编辑器跳转到 `file[:line[:column]]` 的命令行参数列表（精确复刻 Bun `Editor.open`）
pub fn buildOpenEditorCommand(
    editor: EditorKind,
    binary: []const u8,
    file: []const u8,
    line: []const u8,
    column: []const u8,
    platform: OpenerPlatform,
    scratch: []u8,
): EditorOpenCommand! {
    if (binary.len == 0) {
        return err.EmptyBinary;
    }
    if (file.len == 0) {
        return err.EmptyFile;
    }

    let cmd = EditorOpenCommand{};
    let argc: usize = 0;
    let pos: usize = 0;

    if (editor.isTerminalEditor()) {
        cmd.args[argc] = platformOpener(platform);
        argc += 1;
        cmd.args[argc] = binary;
        argc += 1;
        if (platform == .MacOS) {
            cmd.args[argc] = "--args";
            argc += 1;
        }
    }

    cmd.args[argc] = binary;
    argc += 1;

    if (editor == .VsCode && line.len > 0) {
        cmd.args[argc] = "--goto";
        argc += 1;
    }

    if (editor == .Sublime || editor == .Atom || editor == .VsCode || editor == .WebStorm || editor == .IntelliJ) {
        const start = pos;
        pos = appendBytes(scratch, pos, file)?;
        if (line.len > 0) {
            pos = appendBytes(scratch, pos, ":")?;
            pos = appendBytes(scratch, pos, line)?;
            if (!editor.isJetBrains() && column.len > 0) {
                pos = appendBytes(scratch, pos, ":")?;
                pos = appendBytes(scratch, pos, column)?;
            }
        }
        cmd.args[argc] = scratch[start..pos];
        argc += 1;
    } else if (editor == .TextMate) {
        const file_start = pos;
        pos = appendBytes(scratch, pos, file)?;
        const file_slice = scratch[file_start..pos];

        if (line.len > 0) {
            cmd.args[argc] = "--line";
            argc += 1;

            const lc_start = pos;
            pos = appendBytes(scratch, pos, line)?;
            if (column.len > 0) {
                pos = appendBytes(scratch, pos, ":")?;
                pos = appendBytes(scratch, pos, column)?;
            }
            cmd.args[argc] = scratch[lc_start..pos];
            argc += 1;
        }

        cmd.args[argc] = file_slice;
        argc += 1;
    } else {
        const start = pos;
        pos = appendBytes(scratch, pos, file)?;
        cmd.args[argc] = scratch[start..pos];
        argc += 1;
    }

    cmd.argc = argc;
    cmd.scratch_used = pos;
    return cmd;
}

/// 归一化待在编辑器中打开的临时文件名：若以 `.bun` 结尾且长度小于 499，自动追加 `.js` 以启用语法高亮
pub fn normalizeEditorTempFilename(id: []const u8, out: []u8): usize! {
    const base = pathBasename(id);
    let pos = appendBytes(out, 0, base)?;
    if (endsWith(base, ".bun") && base.len < 499) {
        pos = appendBytes(out, pos, ".js")?;
    }
    return pos;
}

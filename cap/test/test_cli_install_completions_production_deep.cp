import { test, expect } from "cap:test";
import {
    CompletionInstallEnv,
    CompletionShell,
    collectAliasSymlinkTargets,
    collectCompletionDirs,
    completionFilename,
    formatZshrcSourceLine,
    isWindowsInstallerBinDir,
    resolveZshrcPath,
    zshrcNeedsCompletionHint,
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

test("cli install_completions: canonical filenames for fish, zsh, bash and error on unsupported", () => {
    let out: [64]u8 = [0; 64];

    const n1 = completionFilename(CompletionShell.fish, "bun", out[0..]) catch 0;
    expect(sliceEq(out[0..n1], "bun.fish")).toBe(true);

    const n2 = completionFilename(CompletionShell.zsh, "cap", out[0..]) catch 0;
    expect(sliceEq(out[0..n2], "_cap")).toBe(true);

    const n3 = completionFilename(CompletionShell.bash, "bun", out[0..]) catch 0;
    expect(sliceEq(out[0..n3], "bun.completion.bash")).toBe(true);

    let unsupported = false;
    * = completionFilename(CompletionShell.unknown, "bun", out[0..]) catch {
        unsupported = true;
    };
    expect(unsupported).toBe(true);
});

test("cli install_completions: fish candidate directories with XDG and macOS ARM64 vs x64", () => {
    const env_arm = CompletionInstallEnv{
        home: "/Users/alice",
        xdg_config_home: "/Users/alice/.xdg",
        is_macos: true,
        is_arm64: true,
    };
    const dirs = collectCompletionDirs(CompletionShell.fish, env_arm) catch {
        expect(false).toBe(true);
        return;
    };
    expect(dirs.count).toBe(4);
    expect(sliceEq(dirs.at(0) ?? "", "/Users/alice/.xdg/fish/completions")).toBe(true);
    expect(sliceEq(dirs.at(1) ?? "", "/Users/alice/.config/fish/completions")).toBe(true);
    expect(sliceEq(dirs.at(2) ?? "", "/opt/homebrew/share/fish/completions")).toBe(true);
    expect(sliceEq(dirs.at(3) ?? "", "/etc/fish/completions")).toBe(true);

    const env_x64 = CompletionInstallEnv{
        is_macos: true,
        is_arm64: false,
    };
    const dirs_x64 = collectCompletionDirs(CompletionShell.fish, env_x64) catch {
        expect(false).toBe(true);
        return;
    };
    expect(sliceEq(dirs_x64.at(0) ?? "", "/usr/local/share/fish/completions")).toBe(true);
});

test("cli install_completions: zsh candidate directories honor $fpath first", () => {
    const env = CompletionInstallEnv{
        home: "/home/bob",
        fpath: "/home/bob/.zfunc  /custom/zsh/fns",
        xdg_data_home: "/home/bob/.local/share",
        install_dir: "/home/bob/.bun",
    };
    const dirs = collectCompletionDirs(CompletionShell.zsh, env) catch {
        expect(false).toBe(true);
        return;
    };
    expect(sliceEq(dirs.at(0) ?? "", "/home/bob/.zfunc")).toBe(true);
    expect(sliceEq(dirs.at(1) ?? "", "/custom/zsh/fns")).toBe(true);
    expect(sliceEq(dirs.at(2) ?? "", "/home/bob/.local/share/zsh-completions")).toBe(true);
    expect(sliceEq(dirs.at(3) ?? "", "/home/bob/.bun")).toBe(true);
    expect(sliceEq(dirs.at(4) ?? "", "/home/bob/.oh-my-zsh/completions")).toBe(true);
});

test("cli install_completions: bash candidate directories order", () => {
    const env = CompletionInstallEnv{
        home: "/home/carol",
        xdg_data_home: "/home/carol/.local/share",
        xdg_config_home: "/home/carol/.config",
    };
    const dirs = collectCompletionDirs(CompletionShell.bash, env) catch {
        expect(false).toBe(true);
        return;
    };
    expect(dirs.count).toBe(6);
    expect(sliceEq(dirs.at(0) ?? "", "/home/carol/.local/share/bash-completion/completions")).toBe(true);
    expect(sliceEq(dirs.at(1) ?? "", "/home/carol/.config/bash-completion/completions")).toBe(true);
    expect(sliceEq(dirs.at(2) ?? "", "/home/carol/.oh-my-bash/custom/completions")).toBe(true);
    expect(sliceEq(dirs.at(3) ?? "", "/home/carol/.bash_completion.d")).toBe(true);
});

test("cli install_completions: ZDOTDIR/.zshrc resolution, hint detection, and source line", () => {
    let out: [256]u8 = [0; 256];

    const env_zdot = CompletionInstallEnv{
        home: "/Users/dev",
        zdotdir: "/Users/dev/.config/zsh/",
    };
    const n1 = resolveZshrcPath(env_zdot, out[0..]) catch 0;
    expect(sliceEq(out[0..n1], "/Users/dev/.config/zsh/.zshrc")).toBe(true);

    const env_home = CompletionInstallEnv{ home: "/Users/dev" };
    const n2 = resolveZshrcPath(env_home, out[0..]) catch 0;
    expect(sliceEq(out[0..n2], "/Users/dev/.zshrc")).toBe(true);

    const comp_path = "/Users/dev/.bun/_bun";
    expect(zshrcNeedsCompletionHint("export PATH=$PATH:/usr/local/bin\n", comp_path)).toBe(true);
    expect(zshrcNeedsCompletionHint("[ -s \"/Users/dev/.bun/_bun\" ] && source \"/Users/dev/.bun/_bun\"\n", comp_path)).toBe(false);

    const n3 = formatZshrcSourceLine(comp_path, out[0..]) catch 0;
    expect(sliceEq(out[0..n3], "[ -s \"/Users/dev/.bun/_bun\" ] && source \"/Users/dev/.bun/_bun\"\n")).toBe(true);
});

test("cli install_completions: alias symlink fallback chain, Windows bin dir check, and error guards", () => {
    const env = CompletionInstallEnv{
        home: "/home/dev",
        install_dir: "/opt/bun",
    };
    const targets = collectAliasSymlinkTargets("/usr/local/bin", "bunx", env) catch {
        expect(false).toBe(true);
        return;
    };
    expect(targets.count).toBe(4);
    expect(sliceEq(targets.at(0) ?? "", "/usr/local/bin/bunx")).toBe(true);
    expect(sliceEq(targets.at(1) ?? "", "/opt/bun/bin/bunx")).toBe(true);
    expect(sliceEq(targets.at(2) ?? "", "/home/dev/.bun/bin/bunx")).toBe(true);
    expect(sliceEq(targets.at(3) ?? "", "/home/dev/.local/bin/bunx")).toBe(true);

    expect(isWindowsInstallerBinDir("C:\\Users\\dev\\.bun\\bin")).toBe(true);
    expect(isWindowsInstallerBinDir("C:\\Users\\dev\\.bun\\bin\\")).toBe(true);
    expect(isWindowsInstallerBinDir("C:\\Program Files\\other")).toBe(false);

    // Error guards
    let tiny: [4]u8 = [0; 4];
    let buf_err = false;
    * = formatZshrcSourceLine("/a/b/_bun", tiny[0..]) catch {
        buf_err = true;
    };
    expect(buf_err).toBe(true);

    let no_home_err = false;
    * = resolveZshrcPath(CompletionInstallEnv{}, tiny[0..]) catch {
        no_home_err = true;
    };
    expect(no_home_err).toBe(true);
});

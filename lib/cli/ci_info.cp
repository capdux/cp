// Source: Absorbed from /data/dev/bun/src/cli/ci_info.cp | Synced: 2026-11-20T21:44:56Z
// lib/cli/ci_info.cp
// Production CI/CD Vendor Detector, PR Detection, ANSI/Hyperlink Probing & Terminal Dimensions

const std = @import("std");

@c struct CiWinSize {
    ws_row: u16 = 0;
    ws_col: u16 = 0;
    ws_xpixel: u16 = 0;
    ws_ypixel: u16 = 0;
}

pub enum CiVendor: u8 {
    None = 0,
    GitHubActions = 1,
    GitLabCi = 2,
    CircleCi = 3,
    Travis = 4,
    Jenkins = 5,
    Buildkite = 6,
    AzurePipelines = 7,
    Vercel = 8,
    Netlify = 9,
    CloudflarePages = 10,
    Bitbucket = 11,
    TeamCity = 12,
    CodeBuild = 13,
    Drone = 14,
    GenericCi = 15,

    pub fn name(): []const u8 {
        return switch (_) {
            .None => "none",
            .GitHubActions => "GitHub Actions",
            .GitLabCi => "GitLab CI",
            .CircleCi => "CircleCI",
            .Travis => "Travis CI",
            .Jenkins => "Jenkins",
            .Buildkite => "Buildkite",
            .AzurePipelines => "Azure Pipelines",
            .Vercel => "Vercel",
            .Netlify => "Netlify CI",
            .CloudflarePages => "Cloudflare Pages",
            .Bitbucket => "Bitbucket Pipelines",
            .TeamCity => "TeamCity",
            .CodeBuild => "AWS CodeBuild",
            .Drone => "Drone",
            .GenericCi => "Generic CI",
        };
    }

    pub fn slug(): []const u8 {
        return switch (_) {
            .None => "none",
            .GitHubActions => "github-actions",
            .GitLabCi => "gitlab-ci",
            .CircleCi => "circleci",
            .Travis => "travis-ci",
            .Jenkins => "jenkins",
            .Buildkite => "buildkite",
            .AzurePipelines => "azure-pipelines",
            .Vercel => "vercel",
            .Netlify => "netlify-ci",
            .CloudflarePages => "cloudflare-pages",
            .Bitbucket => "bitbucket-pipelines",
            .TeamCity => "teamcity",
            .CodeBuild => "aws-codebuild",
            .Drone => "drone",
            .GenericCi => "ci",
        };
    }

    pub fn isKnown(): bool {
        return _ != .None;
    }
}

pub struct CiEnvEntry {
    pub key: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct CiTerminalDim {
    pub width: u16 = 80;
    pub height: u16 = 24;
    pub is_tty: bool = false;
}

pub struct CiInfo {
    pub is_ci: bool = false;
    pub vendor: CiVendor = .None;
    pub vendor_name: []const u8 = "none",
    pub is_pr: bool = false;
    pub supports_ansi: bool = false;
    pub supports_color: bool = false;
    pub supports_truecolor: bool = false;
    pub supports_hyperlink: bool = false;
    pub terminal_width: u16 = 80;
    pub terminal_height: u16 = 24;
    pub is_tty: bool = false;

    /// Queries real terminal dimensions via Linux `ioctl(TIOCGWINSZ)` directly via Linux syscall.
    pub static fn probeTerminal(fd: i32 = 1): CiTerminalDim {
        if (fd < 0) {
            return CiTerminalDim{ width: 80, height: 24, is_tty: false };
        }
        const tty_ok = std.c.isatty(@intCast(fd)) != 0;
        let ws = CiWinSize{};
        let got_cols: u16 = 80;
        let got_rows: u16 = 24;
        zig {
            const rc = std.os.linux.ioctl(@intCast(fd), std.os.linux.T.IOCGWINSZ, @intFromPtr(&ws));
            if (rc == 0 and ws.ws_col > 0 and ws.ws_row > 0) {
                got_cols = ws.ws_col;
                got_rows = ws.ws_row;
            }
        }
        return CiTerminalDim{
            width: got_cols,
            height: got_rows,
            is_tty: tty_ok,
        };
    }

    static fn envGet(entries: []const CiEnvEntry, use_process_env: bool, key: []const u8): ([]const u8)? {
        if (!use_process_env) {
            let i: usize = 0;
            while (i < entries.len) : (i += 1) {
                if (std.mem.eql(u8, entries[i].key, key)) {
                    return entries[i].value;
                }
            }
            return null;
        }
        if (key.len == 0 || key.len >= 128) {
            return null;
        }
        let zbuf: [128]u8 = [0; 128];
        let k: usize = 0;
        while (k < key.len) : (k += 1) {
            zbuf[k] = key[k];
        }
        zbuf[key.len] = 0;
        zig {
            const key_z: [*:0]const u8 = @ptrCast(&zbuf[0]);
            if (std.c.getenv(key_z)) |ptr| {
                return std.mem.span(ptr);
            }
        }
        return null;
    }

    static fn envIsSet(entries: []const CiEnvEntry, use_process_env: bool, key: []const u8): bool {
        if (val := CiInfo.envGet(entries, use_process_env, key)) {
            return val.len > 0;
        }
        return false;
    }

    static fn envEquals(entries: []const CiEnvEntry, use_process_env: bool, key: []const u8, expected: []const u8): bool {
        if (val := CiInfo.envGet(entries, use_process_env, key)) {
            return std.mem.eql(u8, val, expected);
        }
        return false;
    }

    static fn envNotFalse(entries: []const CiEnvEntry, use_process_env: bool, key: []const u8): bool {
        if (val := CiInfo.envGet(entries, use_process_env, key)) {
            return val.len > 0 && !std.mem.eql(u8, val, "false") && !std.mem.eql(u8, val, "0");
        }
        return false;
    }

    static fn detectVendorInternal(entries: []const CiEnvEntry, use_process_env: bool): CiVendor {
        if (CiInfo.envEquals(entries, use_process_env, "CI", "false") ||
            CiInfo.envEquals(entries, use_process_env, "CI", "0"))
        {
            return .None;
        }

        if (CiInfo.envIsSet(entries, use_process_env, "GITHUB_ACTIONS")) {
            return .GitHubActions;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "GITLAB_CI")) {
            return .GitLabCi;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "CIRCLECI")) {
            return .CircleCi;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "TRAVIS")) {
            return .Travis;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "JENKINS_URL") &&
            CiInfo.envIsSet(entries, use_process_env, "BUILD_ID"))
        {
            return .Jenkins;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "BUILDKITE")) {
            return .Buildkite;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "TF_BUILD")) {
            return .AzurePipelines;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "VERCEL") ||
            CiInfo.envIsSet(entries, use_process_env, "NOW_BUILDER"))
        {
            return .Vercel;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "NETLIFY")) {
            return .Netlify;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "CF_PAGES")) {
            return .CloudflarePages;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "BITBUCKET_COMMIT")) {
            return .Bitbucket;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "TEAMCITY_VERSION")) {
            return .TeamCity;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "CODEBUILD_BUILD_ARN")) {
            return .CodeBuild;
        }
        if (CiInfo.envIsSet(entries, use_process_env, "DRONE")) {
            return .Drone;
        }

        const generic_keys: [9][]const u8 = [
            "CI",
            "CONTINUOUS_INTEGRATION",
            "BUILD_NUMBER",
            "BUILD_ID",
            "RUN_ID",
            "CI_NAME",
            "CI_APP_ID",
            "CI_BUILD_ID",
            "CI_BUILD_NUMBER",
        ];
        let gi: usize = 0;
        while (gi < generic_keys.len) : (gi += 1) {
            if (CiInfo.envIsSet(entries, use_process_env, generic_keys[gi])) {
                return .GenericCi;
            }
        }
        return .None;
    }

    static fn detectPrInternal(vendor: CiVendor, entries: []const CiEnvEntry, use_process_env: bool): bool {
        return switch (vendor) {
            .GitHubActions => CiInfo.envEquals(entries, use_process_env, "GITHUB_EVENT_NAME", "pull_request") ||
                CiInfo.envEquals(entries, use_process_env, "GITHUB_EVENT_NAME", "pull_request_target"),
            .GitLabCi => CiInfo.envIsSet(entries, use_process_env, "CI_MERGE_REQUEST_ID") ||
                CiInfo.envIsSet(entries, use_process_env, "CI_MERGE_REQUEST_IID"),
            .CircleCi => CiInfo.envIsSet(entries, use_process_env, "CIRCLE_PULL_REQUEST"),
            .Travis => CiInfo.envNotFalse(entries, use_process_env, "TRAVIS_PULL_REQUEST"),
            .Jenkins => CiInfo.envIsSet(entries, use_process_env, "CHANGE_ID") ||
                CiInfo.envIsSet(entries, use_process_env, "ghprbPullId"),
            .Buildkite => CiInfo.envNotFalse(entries, use_process_env, "BUILDKITE_PULL_REQUEST"),
            .AzurePipelines => CiInfo.envEquals(entries, use_process_env, "BUILD_REASON", "PullRequest"),
            .Vercel => CiInfo.envIsSet(entries, use_process_env, "VERCEL_GIT_PULL_REQUEST_ID"),
            .Netlify => CiInfo.envNotFalse(entries, use_process_env, "PULL_REQUEST"),
            .Bitbucket => CiInfo.envIsSet(entries, use_process_env, "BITBUCKET_PR_ID"),
            .CodeBuild => CiInfo.envEquals(entries, use_process_env, "CODEBUILD_WEBHOOK_EVENT", "PULL_REQUEST_CREATED") ||
                CiInfo.envEquals(entries, use_process_env, "CODEBUILD_WEBHOOK_EVENT", "PULL_REQUEST_UPDATED") ||
                CiInfo.envEquals(entries, use_process_env, "CODEBUILD_WEBHOOK_EVENT", "PULL_REQUEST_REOPENED"),
            .Drone => CiInfo.envEquals(entries, use_process_env, "DRONE_BUILD_EVENT", "pull_request"),
            else => false,
        };
    }

    static fn detectInternal(
        entries: []const CiEnvEntry,
        use_process_env: bool,
        dim: CiTerminalDim,
    ): CiInfo {
        const vendor = CiInfo.detectVendorInternal(entries, use_process_env);
        const is_ci = vendor != .None;
        const is_pr = if (is_ci) CiInfo.detectPrInternal(vendor, entries, use_process_env) else false;

        const no_color = CiInfo.envIsSet(entries, use_process_env, "NO_COLOR");
        const force_color = CiInfo.envNotFalse(entries, use_process_env, "FORCE_COLOR");
        const is_dumb = CiInfo.envEquals(entries, use_process_env, "TERM", "dumb");

        const supports_ansi = if (no_color || is_dumb)
            false
        else if (force_color)
            true
        else if (dim.is_tty)
            true
        else
            (vendor == .GitHubActions || vendor == .GitLabCi || vendor == .CircleCi || vendor == .Buildkite || vendor == .Travis);

        const ct = CiInfo.envGet(entries, use_process_env, "COLORTERM") ?? "";
        const supports_truecolor = !no_color && !is_dumb &&
            (std.mem.eql(u8, ct, "truecolor") || std.mem.eql(u8, ct, "24bit"));

        const term_prog = CiInfo.envGet(entries, use_process_env, "TERM_PROGRAM") ?? "";
        const supports_hyperlink = supports_ansi && (
            vendor == .GitHubActions ||
            CiInfo.envIsSet(entries, use_process_env, "WT_SESSION") ||
            CiInfo.envIsSet(entries, use_process_env, "KITTY_WINDOW_ID") ||
            std.mem.eql(u8, term_prog, "iTerm.app") ||
            std.mem.eql(u8, term_prog, "WezTerm") ||
            std.mem.eql(u8, term_prog, "vscode")
        );

        let cols = dim.width;
        if (col_str := CiInfo.envGet(entries, use_process_env, "COLUMNS")) {
            if (parsed_cols := std.fmt.parseInt(u16, col_str, 10) catch null) {
                if (parsed_cols > 0) {
                    cols = parsed_cols;
                }
            }
        }

        return CiInfo{
            is_ci: is_ci,
            vendor: vendor,
            vendor_name: vendor.name(),
            is_pr: is_pr,
            supports_ansi: supports_ansi,
            supports_color: supports_ansi,
            supports_truecolor: supports_truecolor,
            supports_hyperlink: supports_hyperlink,
            terminal_width: cols,
            terminal_height: dim.height,
            is_tty: dim.is_tty,
        };
    }

    /// Detects CI vendor, PR status, ANSI/Hyperlink capabilities, and terminal size from live process env & TTY.
    pub static fn detect(fd: i32 = 1): CiInfo {
        const empty_env: []const CiEnvEntry = [];
        const dim = CiInfo.probeTerminal(fd);
        return CiInfo.detectInternal(empty_env, true, dim);
    }

    /// Deterministic detection against explicit environment pairs (ideal for isolated tests & simulations).
    pub static fn detectWithEnv(
        entries: []const CiEnvEntry,
        is_tty: bool = false,
        width: u16 = 80,
        height: u16 = 24,
    ): CiInfo {
        return CiInfo.detectInternal(entries, false, CiTerminalDim{
            width: if (width == 0) 80 else width,
            height: if (height == 0) 24 else height,
            is_tty: is_tty,
        });
    }
}

/// Bun-compatible top-level helper: returns `true` if running in a CI environment.
pub fn isCI(): bool {
    return CiInfo.detect(1).is_ci;
}

/// Bun-compatible top-level helper: returns CI vendor name or `null` if unknown/not CI.
pub fn detectCIName(): ([]const u8)? {
    const info = CiInfo.detect(1);
    if (!info.is_ci || info.vendor == .GenericCi) {
        return null;
    }
    return info.vendor_name;
}

// Source: Absorbed from _ref/puppeteer/packages/browsers/src/launch.ts | Synced: 2026-11-28
// ============================================================================
// lib/cdp/launcher.cp — Browser Launch Options, Executable Probing &
// DevTools WebSocket URL Parser (`DevTools listening on ws://...`)
// ============================================================================

const std = @import("std");
import { CdpError, CdpFrameScanner } from "./protocol";

pub struct CdpBrowserLaunchOptions {
    headless: bool = true,
    executable_path: []const u8 = "",
    user_data_dir: []const u8 = "",
    args: []const []const u8 = &.{},
    viewport_width: u32 = 1280,
    viewport_height: u32 = 720,
    timeout_ms: u32 = 30000,
    remote_debugging_port: u16 = 0,
    ignore_https_errors: bool = false,
}

pub struct CdpWsEndpoint {
    url: []const u8 = "",
    host: []const u8 = "",
    port: u16 = 0,
    path: []const u8 = "",
    is_tls: bool = false,
}

pub struct CdpBrowserLauncher {
    /// Default system browser binary names to probe in priority order.
    pub static fn defaultCandidateNames(): []const []const u8 {
        return &.{
            "chromium",
            "google-chrome",
            "chromium-browser",
            "msedge",
            "capsurf",
        };
    }

    /// Standard Linux/macOS binary directories for browser probing.
    pub static fn defaultSearchDirs(): []const []const u8 {
        return &.{
            "/usr/bin",
            "/usr/local/bin",
            "/snap/bin",
            "/opt/google/chrome",
            "/opt/microsoft/msedge",
        };
    }

    static fn isExecutableReadable(path: []const u8): bool {
        if (path.len == 0 || path.len >= 4096) return false;
        let zbuf: [4097]u8 = undefined;
        @memcpy(zbuf[0..path.len], path);
        zbuf[path.len] = 0;
        const zptr: [*:0]const u8 = unsafe { @ptrCast(&zbuf[0]) };
        return std.c.access(zptr, 1) == 0;
    }

    /// Probe for a browser executable. Checks `preferred` first, then standard directories.
    pub static fn probeBrowserExecutable(alloc: std.mem.Allocator, preferred: []const u8): ([]u8)? {
        if (preferred.len > 0) {
            if (CdpBrowserLauncher.isExecutableReadable(preferred)) {
                return alloc.dupe(u8, preferred) catch null;
            }
        }

        const dirs = CdpBrowserLauncher.defaultSearchDirs();
        const names = CdpBrowserLauncher.defaultCandidateNames();
        for (dir of dirs) {
            for (name of names) {
                const candidate = std.fmt.allocPrint(alloc, "{s}/{s}", .{ dir, name }) catch continue;
                if (CdpBrowserLauncher.isExecutableReadable(candidate)) {
                    return candidate;
                }
                alloc.free(candidate);
            }
        }
        return null;
    }

    /// Build Chromium/Capsurf CLI launch flags from CdpBrowserLaunchOptions.
    pub static fn buildLaunchArgs(alloc: std.mem.Allocator, opts: CdpBrowserLaunchOptions): ([]([]u8))!CdpError {
        let out: std.ArrayList([]u8) = .empty;

        const base_flags: []const []const u8 = &.{
            "--no-first-run",
            "--no-default-browser-check",
            "--disable-background-networking",
            "--disable-background-timer-throttling",
            "--disable-breakpad",
            "--disable-component-update",
            "--disable-dev-shm-usage",
            "--disable-extensions",
            "--disable-hang-monitor",
            "--disable-popup-blocking",
            "--disable-prompt-on-repost",
            "--disable-sync",
            "--metrics-recording-only",
            "--password-store=basic",
            "--use-mock-keychain",
        };

        for (flag of base_flags) {
            const duped = alloc.dupe(u8, flag) catch return CdpError.OutOfMemory;
            out.append(alloc, duped) catch return CdpError.OutOfMemory;
        }

        if (opts.headless) {
            const hf = alloc.dupe(u8, "--headless=new") catch return CdpError.OutOfMemory;
            out.append(alloc, hf) catch return CdpError.OutOfMemory;
        }

        const port_flag = std.fmt.allocPrint(
            alloc,
            "--remote-debugging-port={d}",
            .{opts.remote_debugging_port},
        ) catch return CdpError.OutOfMemory;
        out.append(alloc, port_flag) catch return CdpError.OutOfMemory;

        const win_flag = std.fmt.allocPrint(
            alloc,
            "--window-size={d},{d}",
            .{ opts.viewport_width, opts.viewport_height },
        ) catch return CdpError.OutOfMemory;
        out.append(alloc, win_flag) catch return CdpError.OutOfMemory;

        if (opts.user_data_dir.len > 0) {
            const udd_flag = std.fmt.allocPrint(
                alloc,
                "--user-data-dir={s}",
                .{opts.user_data_dir},
            ) catch return CdpError.OutOfMemory;
            out.append(alloc, udd_flag) catch return CdpError.OutOfMemory;
        }

        if (opts.ignore_https_errors) {
            const ig = alloc.dupe(u8, "--ignore-certificate-errors") catch return CdpError.OutOfMemory;
            out.append(alloc, ig) catch return CdpError.OutOfMemory;
        }

        for (extra of opts.args) {
            const duped = alloc.dupe(u8, extra) catch return CdpError.OutOfMemory;
            out.append(alloc, duped) catch return CdpError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return CdpError.OutOfMemory;
    }

    pub static fn freeLaunchArgs(alloc: std.mem.Allocator, args: [][]u8) {
        for (a of args) {
            alloc.free(a);
        }
        alloc.free(args);
    }

    /// Parse `DevTools listening on ws://127.0.0.1:9222/devtools/browser/...` from stderr output.
    pub static fn parseDevToolsWsUrl(stderr_chunk: []const u8): CdpWsEndpoint? {
        const prefix = "DevTools listening on ";
        const idx = std.mem.indexOf(u8, stderr_chunk, prefix) orelse return null;
        let url_start = idx + prefix.len;
        while (url_start < stderr_chunk.len && (stderr_chunk[url_start] == ' ' || stderr_chunk[url_start] == '\t')) {
            url_start += 1;
        }

        let url_end = url_start;
        while (url_end < stderr_chunk.len) {
            const c = stderr_chunk[url_end];
            if (c == '\r' || c == '\n' || c == ' ' || c == '\t') {
                break;
            }
            url_end += 1;
        }

        const full_url = stderr_chunk[url_start..url_end];
        return CdpBrowserLauncher.parseWsUrl(full_url);
    }

    /// Parse a `ws://host:port/path` or `wss://host:port/path` URL into CdpWsEndpoint.
    pub static fn parseWsUrl(url: []const u8): CdpWsEndpoint? {
        let is_tls = false;
        let rest: []const u8 = "";
        if (std.mem.startsWith(u8, url, "ws://")) {
            rest = url[5..];
        } else if (std.mem.startsWith(u8, url, "wss://")) {
            is_tls = true;
            rest = url[6..];
        } else {
            return null;
        }

        const slash_idx = std.mem.indexOfScalar(u8, rest, '/') orelse rest.len;
        const authority = rest[0..slash_idx];
        const path = if (slash_idx < rest.len) rest[slash_idx..] else "/";

        if (authority.len == 0) return null;

        let host = authority;
        let port: u16 = if (is_tls) 443 else 80;

        // Handle IPv6 `[::1]:9222` or IPv4 `127.0.0.1:9222`
        if (authority[0] == '[') {
            if (close_br := std.mem.indexOfScalar(u8, authority, ']')) {
                host = authority[0 .. close_br + 1];
                if (close_br + 1 < authority.len && authority[close_br + 1] == ':') {
                    const p_val = CdpFrameScanner.parseFastI64(authority[close_br + 2 ..]) orelse return null;
                    if (p_val < 0 || p_val > 65535) return null;
                    port = p_val as u16;
                }
            } else {
                return null;
            }
        } else if (colon_idx := std.mem.lastIndexOfScalar(u8, authority, ':')) {
            host = authority[0..colon_idx];
            const p_val = CdpFrameScanner.parseFastI64(authority[colon_idx + 1 ..]) orelse return null;
            if (p_val < 0 || p_val > 65535) return null;
            port = p_val as u16;
        }

        return CdpWsEndpoint{
            url: url,
            host: host,
            port: port,
            path: path,
            is_tls: is_tls,
        };
    }
}

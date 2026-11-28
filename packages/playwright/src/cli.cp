// Source: Absorbed from _ref/playwright/packages/playwright-core/src/cli/browserActions.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/cli.cp — Playwright CLI Action Parser & Executor
// Implements `open`, `screenshot`, `pdf`, and `codegen` actions directly on
// native CP ChromiumBrowserType/Page without Node.js driver overhead.
// ============================================================================

const std = @import("std");
import { CdpSendTransportFn } from "cap:cdp";
import { PlaywrightError } from "./network";
import { GotoOptions, ScreenshotOptions, PdfOptions } from "./page";
import { ChromiumBrowserType, LaunchOptions } from "./chromium";

pub enum CliCommand {
    Open,
    Screenshot,
    Pdf,
    Codegen,
    Help,
    Unknown,
}

pub struct CliOptions {
    pub command: CliCommand = CliCommand.Help,
    pub url: []const u8 = "about:blank",
    pub output_path: []const u8 = "",
    pub browser: []const u8 = "chromium",
    pub headless: bool = true,
    pub full_page: bool = false,
    pub viewport_width: u32 = 1280,
    pub viewport_height: u32 = 720,
    pub target_lang: []const u8 = "cp",
    pub landscape: bool = false,
}

pub struct CliResult {
    allocator: std.mem.Allocator = undefined,
    pub command: CliCommand = CliCommand.Help,
    pub ok: bool = true,
    pub final_url: []const u8 = "",
    pub generated_code: []u8 = &.{},
    pub artifact_bytes: usize = 0,

    pub fn deinit(*_) {
        if (_.generated_code.len > 0) {
            _.allocator.free(_.generated_code);
            _.generated_code = &.{};
        }
    }

    ~init() {
        _.deinit();
    }
}

pub struct PlaywrightCli {
    pub static fn parseArgs(args: []const []const u8): CliOptions!PlaywrightError {
        let opts = CliOptions{};
        if (args.len == 0) return opts;

        const cmd = args[0];
        if (std.mem.eql(u8, cmd, "open")) {
            opts.command = CliCommand.Open;
            opts.headless = false;
        } else if (std.mem.eql(u8, cmd, "screenshot")) {
            opts.command = CliCommand.Screenshot;
            opts.output_path = "screenshot.png";
        } else if (std.mem.eql(u8, cmd, "pdf")) {
            opts.command = CliCommand.Pdf;
            opts.output_path = "page.pdf";
        } else if (std.mem.eql(u8, cmd, "codegen")) {
            opts.command = CliCommand.Codegen;
            opts.headless = false;
        } else if (std.mem.eql(u8, cmd, "help") || std.mem.eql(u8, cmd, "--help") || std.mem.eql(u8, cmd, "-h")) {
            opts.command = CliCommand.Help;
        } else {
            opts.command = CliCommand.Unknown;
        }

        let i: usize = 1;
        let positional: usize = 0;
        while (i < args.len) : (i += 1) {
            const a = args[i];
            if (std.mem.eql(u8, a, "--full-page")) {
                opts.full_page = true;
            } else if (std.mem.eql(u8, a, "--headless")) {
                opts.headless = true;
            } else if (std.mem.eql(u8, a, "--headed")) {
                opts.headless = false;
            } else if (std.mem.eql(u8, a, "--landscape")) {
                opts.landscape = true;
            } else if (std.mem.startsWith(u8, a, "--target=")) {
                opts.target_lang = a[9..];
            } else if (std.mem.startsWith(u8, a, "--browser=")) {
                opts.browser = a[10..];
            } else if (!std.mem.startsWith(u8, a, "-")) {
                if (positional == 0) {
                    opts.url = a;
                } else if (positional == 1) {
                    opts.output_path = a;
                }
                positional += 1;
            }
        }
        return opts;
    }

    pub static fn generateCodegenSkeleton(alloc: std.mem.Allocator, opts: CliOptions): ([]u8)!PlaywrightError {
        if (std.mem.eql(u8, opts.target_lang, "ts") || std.mem.eql(u8, opts.target_lang, "typescript")) {
            return std.fmt.allocPrint(
                alloc,
                "import {{ chromium }} from 'playwright';\n\n(async () => {{\n  const browser = await chromium.launch({{ headless: false }});\n  const page = await browser.newPage();\n  await page.goto('{s}');\n  await browser.close();\n}})();\n",
                .{opts.url},
            ) catch return PlaywrightError.OutOfMemory;
        }
        if (std.mem.eql(u8, opts.target_lang, "python")) {
            return std.fmt.allocPrint(
                alloc,
                "from playwright.sync_api import sync_playwright\n\nwith sync_playwright() as p:\n    browser = p.chromium.launch(headless=False)\n    page = browser.new_page()\n    page.goto(\"{s}\")\n    browser.close()\n",
                .{opts.url},
            ) catch return PlaywrightError.OutOfMemory;
        }
        return std.fmt.allocPrint(
            alloc,
            "const std = @import(\"std\");\nimport {{ chromium }} from \"playwright\";\n\nfn main(): !void {{\n    let gpa: std.heap.DebugAllocator(.{{}}) = .{{}};\n    const alloc = gpa.allocator();\n    let browser = chromium.launch(alloc, .{{ headless: false }})?;\n    defer browser.deinit();\n    const page = browser.newPage()?;\n    * = page.goto(\"{s}\", .{{}})?;\n}}\n",
            .{opts.url},
        ) catch return PlaywrightError.OutOfMemory;
    }

    pub static fn execute(
        alloc: std.mem.Allocator,
        opts: CliOptions,
        transport_fn: CdpSendTransportFn?,
        transport_ctx: usize,
    ): CliResult!PlaywrightError {
        let res = CliResult{
            allocator: alloc,
            command: opts.command,
            ok: true,
            final_url: opts.url,
            generated_code: &.{},
            artifact_bytes: 0,
        };
        errdefer res.deinit();

        if (opts.command == CliCommand.Help) {
            return res;
        }
        if (opts.command == CliCommand.Unknown) {
            return PlaywrightError.InvalidSelector;
        }
        if (opts.command == CliCommand.Codegen) {
            res.generated_code = PlaywrightCli.generateCodegenSkeleton(alloc, opts)?;
            return res;
        }

        let browser = ChromiumBrowserType.launch(alloc, LaunchOptions{
            headless: opts.headless,
            viewport_width: opts.viewport_width,
            viewport_height: opts.viewport_height,
            transport_fn: transport_fn,
            transport_ctx: transport_ctx,
        })?;
        defer browser.deinit();

        const page = browser.newPage()?;
        const nav = page.goto(opts.url, GotoOptions{})?;
        res.final_url = opts.url;
        * = nav;

        if (opts.command == CliCommand.Screenshot) {
            const raw = page.screenshot(ScreenshotOptions{
                format: "png",
                full_page: opts.full_page,
            })?;
            res.artifact_bytes = raw.len;
        } else if (opts.command == CliCommand.Pdf) {
            const raw = page.pdf(PdfOptions{
                landscape: opts.landscape,
            })?;
            res.artifact_bytes = raw.len;
        }

        return res;
    }
}

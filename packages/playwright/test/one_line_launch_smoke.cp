// One-line chromium.launch() real Chromium 158.0.8127.88 end-to-end test
const std = @import("std");
import {
    chromium,
    Browser,
    Page,
    Response,
    LaunchOptions,
} from "../src/index";

pub fn main() {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    let alloc: std.mem.Allocator = gpa.allocator();

    let browser: Browser = chromium.launch(alloc, LaunchOptions{
        headless: true,
        remote_debugging_port: 9232,
        user_data_dir: "/tmp/pw-oneline-9232",
    }) catch {
        std.debug.print("FAIL: chromium.launch failed\n", .{});
        std.process.exit(1);
    };
    defer browser.deinit();

    let page: *Page = browser.newPage() catch {
        std.debug.print("FAIL: browser.newPage failed\n", .{});
        std.process.exit(1);
    };
    * = page.goto("data:text/html,<title>PW-OneLine-158</title><h1>Playwright</h1>", .{}) catch {
        std.debug.print("FAIL: page.goto failed\n", .{});
        std.process.exit(1);
    };

    let title: []const u8 = page.evaluate("document.title") catch "ERR";
    std.debug.print("[PW OneLine Launch] document.title -> {s}\n", .{title});

    browser.close();
}

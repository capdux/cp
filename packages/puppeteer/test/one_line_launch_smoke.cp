// One-line Puppeteer.launch() real Chromium 158.0.8127.88 end-to-end test
const std = @import("std");
import { Puppeteer, Browser, Page, LaunchOptions, EvaluateResult } from "../src/index";

pub fn main() {
    let gpa: std.heap.DebugAllocator(.{}) = .init;
    defer std.debug.assert(gpa.deinit() == .ok);
    let alloc: std.mem.Allocator = gpa.allocator();

    let browser = Puppeteer.launch(alloc, LaunchOptions{
        headless: true,
        remote_debugging_port: 9231,
        user_data_dir: "/tmp/pp-oneline-9231",
    }) catch {
        std.debug.print("FAIL: Puppeteer.launch failed\n", .{});
        std.process.exit(1);
    };
    defer browser.deinit();

    let vbuf: [256]u8 = undefined;
    let ver = browser.version(&vbuf) catch "";
    std.debug.print("[OneLine Launch] Browser.version -> {s}\n", .{ver});

    let page = browser.newPage() catch {
        std.debug.print("FAIL: newPage failed\n", .{});
        std.process.exit(1);
    };
    page.goto("data:text/html,<title>OneLine-Launch-158</title>", .{}) catch {};

    let ebuf: [512]u8 = undefined;
    let res = page.evaluate("document.title", &ebuf) catch EvaluateResult{};
    std.debug.print("[OneLine Launch] document.title -> {s}\n", .{res.value_json});

    browser.close() catch {};
}

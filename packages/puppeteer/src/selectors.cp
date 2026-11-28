// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/common/PSelectorParser.ts, _ref/puppeteer/packages/puppeteer-core/src/common/WaitTask.ts & _ref/puppeteer/packages/puppeteer-core/src/injected/ | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/selectors.cp — PSelectorParser, SIMD Fast-Path,
// Injected Page Util Scripts, Poller & WaitTask
// Zero-heap-allocation fixed-pool parser with @Vector(16, u8) SIMD scanning
// ============================================================================

const std = @import("std");
import { PuppeteerError } from "./types";

pub enum PSelectorKind {
    Css,
    Text,
    XPath,
    Aria,
    Pierce,
    Custom,
}

pub enum PCombinator {
    None,
    Descendent, // >>>
    Child,      // >>>>
}

pub struct PSelectorSegment {
    kind: PSelectorKind = PSelectorKind.Css,
    combinator: PCombinator = PCombinator.None,
    name: []const u8 = "",
    value: []const u8 = "",
}

pub struct ParsedPSelector {
    segments: [16]PSelectorSegment = undefined,
    count: usize = 0,
    is_pure_css: bool = true,
    has_aria: bool = false,
    has_pierce: bool = false,
    unquote_buf: [512]u8 = undefined,
    unquote_used: usize = 0,

    pub fn slice(): []const PSelectorSegment {
        return _.segments[0.._.count];
    }

    pub fn pushSegment(*_, seg: PSelectorSegment) {
        if (_.count < 16) {
            _.segments[_.count] = seg;
            _.count += 1;
        }
    }

    fn allocUnquoted(*_, raw: []const u8): []const u8 {
        if (raw.len == 0) return "";
        let src = raw;
        if (src.len >= 2) {
            const first = src[0];
            const last = src[src.len - 1];
            if ((first == '"' || first == '\'') && last == first) {
                src = src[1 .. src.len - 1];
            }
        }
        const avail = _.unquote_buf.len - _.unquote_used;
        if (avail == 0) return src;
        const start = _.unquote_used;
        let i: usize = 0;
        while (i < src.len && (_.unquote_used - start) < avail) {
            if (src[i] == '\\' && i + 1 < src.len) {
                i += 1;
            }
            _.unquote_buf[_.unquote_used] = src[i];
            _.unquote_used += 1;
            i += 1;
        }
        return _.unquote_buf[start.._.unquote_used];
    }
}

pub struct SimdSelectorScanner {
    /// 16-lane @Vector(16, u8) SIMD scan to detect if a selector may contain
    /// Puppeteer pseudo-elements (`:-p-`), shadow combinators (`>>>`), or prefix engines (`/`).
    pub static fn needsPSelectorParsing(selector: []const u8): bool {
        if (selector.len == 0) return false;
        let has_colon_or_gt_or_slash = false;
        zig {
            const Vec = @Vector(16, u8);
            const v_colon: Vec = @splat(':');
            const v_gt: Vec = @splat('>');
            const v_slash: Vec = @splat('/');
            var i: usize = 0;
            while (i + 16 <= selector.len) {
                var buf: [16]u8 align(16) = undefined;
                @memcpy(&buf, selector[i .. i + 16]);
                const chunk: Vec = buf;
                const m1 = chunk == v_colon;
                const m2 = chunk == v_gt;
                const m3 = chunk == v_slash;
                if (@reduce(.Or, m1) or @reduce(.Or, m2) or @reduce(.Or, m3)) {
                    has_colon_or_gt_or_slash = true;
                    break;
                }
                i += 16;
            }
            if (!has_colon_or_gt_or_slash) {
                while (i < selector.len) {
                    const ch = selector[i];
                    if (ch == ':' or ch == '>' or ch == '/') {
                        has_colon_or_gt_or_slash = true;
                        break;
                    }
                    i += 1;
                }
            }
        }
        if (!has_colon_or_gt_or_slash) return false;

        if (std.mem.indexOf(u8, selector, "::-p-") != null) return true;
        if (std.mem.indexOf(u8, selector, ">>>") != null) return true;
        if (std.mem.startsWith(u8, selector, "aria/")) return true;
        if (std.mem.startsWith(u8, selector, "text/")) return true;
        if (std.mem.startsWith(u8, selector, "xpath/")) return true;
        if (std.mem.startsWith(u8, selector, "pierce/")) return true;
        return false;
    }

    /// 16-lane SIMD indexOfByte for fast delimiter lookup in selector & JSON hot paths.
    pub static fn indexOfByteSimd(haystack: []const u8, needle: u8): usize? {
        let found: usize? = null;
        zig {
            const Vec = @Vector(16, u8);
            const v_needle: Vec = @splat(needle);
            var i: usize = 0;
            while (i + 16 <= haystack.len) {
                var buf: [16]u8 align(16) = undefined;
                @memcpy(&buf, haystack[i .. i + 16]);
                const chunk: Vec = buf;
                const mask = chunk == v_needle;
                if (@reduce(.Or, mask)) {
                    var lane: usize = 0;
                    while (lane < 16) : (lane += 1) {
                        if (haystack[i + lane] == needle) {
                            found = i + lane;
                            break;
                        }
                    }
                    break;
                }
                i += 16;
            }
            if (found == null) {
                while (i < haystack.len) : (i += 1) {
                    if (haystack[i] == needle) {
                        found = i;
                        break;
                    }
                }
            }
        }
        return found;
    }
}

pub struct PSelectorParser {
    pub static fn parse(selector: []const u8): ParsedPSelector {
        let res = ParsedPSelector{};
        const trimmed = std.mem.trim(u8, selector, " \t\r\n");
        if (trimmed.len == 0) return res;

        // Fast-path: pure CSS (checked with @Vector(16, u8) SIMD)
        if (!SimdSelectorScanner.needsPSelectorParsing(trimmed)) {
            res.pushSegment(PSelectorSegment{
                kind: PSelectorKind.Css,
                value: trimmed,
            });
            return res;
        }

        // Legacy prefix engines: aria/, text/, xpath/, pierce/
        if (std.mem.startsWith(u8, trimmed, "aria/")) {
            res.is_pure_css = false;
            res.has_aria = true;
            res.pushSegment(PSelectorSegment{
                kind: PSelectorKind.Aria,
                name: "aria",
                value: res.allocUnquoted(trimmed[5..]),
            });
            return res;
        }
        if (std.mem.startsWith(u8, trimmed, "text/")) {
            res.is_pure_css = false;
            res.pushSegment(PSelectorSegment{
                kind: PSelectorKind.Text,
                name: "text",
                value: res.allocUnquoted(trimmed[5..]),
            });
            return res;
        }
        if (std.mem.startsWith(u8, trimmed, "xpath/")) {
            res.is_pure_css = false;
            res.pushSegment(PSelectorSegment{
                kind: PSelectorKind.XPath,
                name: "xpath",
                value: trimmed[6..],
            });
            return res;
        }
        if (std.mem.startsWith(u8, trimmed, "pierce/")) {
            res.is_pure_css = false;
            res.has_pierce = true;
            res.pushSegment(PSelectorSegment{
                kind: PSelectorKind.Pierce,
                name: "pierce",
                value: std.mem.trim(u8, trimmed[7..], " \t"),
            });
            return res;
        }

        // General P-Selector tokenizing: supports >>>, >>>>, ::-p-text(...), ::-p-xpath(...), ::-p-aria(...)
        let pos: usize = 0;
        let seg_start: usize = 0;
        while (pos < trimmed.len) {
            // Check for >>>> or >>>
            if (pos + 3 <= trimmed.len && trimmed[pos] == '>' && trimmed[pos + 1] == '>' && trimmed[pos + 2] == '>') {
                const prefix = std.mem.trim(u8, trimmed[seg_start..pos], " \t");
                if (prefix.len > 0) {
                    res.pushSegment(PSelectorSegment{ kind: PSelectorKind.Css, value: prefix });
                }
                res.is_pure_css = false;
                res.has_pierce = true;
                if (pos + 4 <= trimmed.len && trimmed[pos + 3] == '>') {
                    res.pushSegment(PSelectorSegment{
                        kind: PSelectorKind.Pierce,
                        combinator: PCombinator.Child,
                        name: "pierce-child",
                    });
                    pos += 4;
                } else {
                    res.pushSegment(PSelectorSegment{
                        kind: PSelectorKind.Pierce,
                        combinator: PCombinator.Descendent,
                        name: "pierce-descendent",
                    });
                    pos += 3;
                }
                while (pos < trimmed.len && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
                    pos += 1;
                }
                seg_start = pos;
                continue;
            }

            // Check for ::-p-<name>(<arg>)
            if (pos + 5 <= trimmed.len && std.mem.eql(u8, trimmed[pos .. pos + 5], "::-p-")) {
                const css_prefix = std.mem.trim(u8, trimmed[seg_start..pos], " \t");
                if (css_prefix.len > 0) {
                    res.pushSegment(PSelectorSegment{ kind: PSelectorKind.Css, value: css_prefix });
                }
                res.is_pure_css = false;
                const name_start = pos + 5;
                let name_end = name_start;
                while (name_end < trimmed.len && trimmed[name_end] != '(' && trimmed[name_end] != ' ') {
                    name_end += 1;
                }
                const pname = trimmed[name_start..name_end];
                let arg_val: []const u8 = "";
                if (name_end < trimmed.len && trimmed[name_end] == '(') {
                    let depth: i32 = 1;
                    let arg_pos = name_end + 1;
                    let in_quote: u8 = 0;
                    while (arg_pos < trimmed.len && depth > 0) {
                        const c = trimmed[arg_pos];
                        if (in_quote != 0) {
                            if (c == '\\' && arg_pos + 1 < trimmed.len) {
                                arg_pos += 2;
                                continue;
                            }
                            if (c == in_quote) in_quote = 0;
                        } else {
                            if (c == '"' || c == '\'') {
                                in_quote = c;
                            } else if (c == '(') {
                                depth += 1;
                            } else if (c == ')') {
                                depth -= 1;
                                if (depth == 0) break;
                            }
                        }
                        arg_pos += 1;
                    }
                    arg_val = res.allocUnquoted(trimmed[name_end + 1 .. arg_pos]);
                    pos = if (arg_pos < trimmed.len) arg_pos + 1 else arg_pos;
                } else {
                    pos = name_end;
                }

                let kind = PSelectorKind.Custom;
                if (std.mem.eql(u8, pname, "text")) {
                    kind = PSelectorKind.Text;
                } else if (std.mem.eql(u8, pname, "xpath")) {
                    kind = PSelectorKind.XPath;
                } else if (std.mem.eql(u8, pname, "aria")) {
                    kind = PSelectorKind.Aria;
                    res.has_aria = true;
                }
                res.pushSegment(PSelectorSegment{
                    kind: kind,
                    name: pname,
                    value: arg_val,
                });
                while (pos < trimmed.len && (trimmed[pos] == ' ' || trimmed[pos] == '\t')) {
                    pos += 1;
                }
                seg_start = pos;
                continue;
            }
            pos += 1;
        }

        if (seg_start < trimmed.len) {
            const tail = std.mem.trim(u8, trimmed[seg_start..], " \t");
            if (tail.len > 0) {
                res.pushSegment(PSelectorSegment{ kind: PSelectorKind.Css, value: tail });
            }
        }
        return res;
    }
}

/// Core Puppeteer page-side injected runtime (queryText, queryXPath, queryPierce, Pollers).
pub const PUPPETEER_INJECTED_SCRIPT: []const u8 =
    "(() => {" ++
    "if (globalThis.__puppeteerUtil) return globalThis.__puppeteerUtil;" ++
    "const queryText = (root, text) => {" ++
    "  const w = document.createTreeWalker(root || document, NodeFilter.SHOW_ELEMENT);" ++
    "  const out = []; let n;" ++
    "  while ((n = w.nextNode())) { if ((n.textContent || '').includes(text)) out.push(n); }" ++
    "  return out;" ++
    "};" ++
    "const queryXPath = (root, expr) => {" ++
    "  const it = document.evaluate(expr, root || document, null, XPathResult.ORDERED_NODE_ITERATOR_TYPE, null);" ++
    "  const out = []; let n; while ((n = it.iterateNext())) out.push(n); return out;" ++
    "};" ++
    "const queryPierce = (root, sel) => {" ++
    "  const out = []; const walk = (r) => {" ++
    "    if (r.querySelectorAll) out.push(...r.querySelectorAll(sel));" ++
    "    const w = document.createTreeWalker(r, NodeFilter.SHOW_ELEMENT); let n;" ++
    "    while ((n = w.nextNode())) { if (n.shadowRoot) walk(n.shadowRoot); }" ++
    "  }; walk(root || document); return out;" ++
    "};" ++
    "const pollRaf = (fn, timeout) => new Promise((res, rej) => {" ++
    "  const start = Date.now();" ++
    "  const step = () => { try { const v = fn(); if (v) return res(v); } catch (e) { return rej(e); }" ++
    "    if (timeout > 0 && Date.now() - start >= timeout) return rej(new Error('Timeout'));" ++
    "    requestAnimationFrame(step); }; step();" ++
    "});" ++
    "const pollInterval = (fn, ms, timeout) => new Promise((res, rej) => {" ++
    "  const start = Date.now();" ++
    "  const step = () => { try { const v = fn(); if (v) return res(v); } catch (e) { return rej(e); }" ++
    "    if (timeout > 0 && Date.now() - start >= timeout) return rej(new Error('Timeout'));" ++
    "    setTimeout(step, ms); }; step();" ++
    "});" ++
    "globalThis.__puppeteerUtil = { queryText, queryXPath, queryPierce, pollRaf, pollInterval };" ++
    "return globalThis.__puppeteerUtil;" ++
    "})()";

pub enum WaitTaskPolling {
    Raf,
    Mutation,
    Interval,
}

pub struct WaitForFunctionOptions {
    polling: WaitTaskPolling = WaitTaskPolling.Raf,
    polling_ms: u32 = 100,
    timeout_ms: u32 = 30000,
}

pub struct WaitTask {
    expression: []const u8 = "",
    options: WaitForFunctionOptions = .{},

    pub static fn init(expression: []const u8, options: WaitForFunctionOptions): WaitTask {
        return WaitTask{
            expression: expression,
            options: options,
        };
    }

    pub fn buildPollerExpression(*_, out_buf: []u8): ([]const u8)!PuppeteerError {
        const poll_call = switch (_.options.polling) {
            WaitTaskPolling.Raf => std.fmt.bufPrint(
                out_buf,
                "(() => {{ const u = {s}; return u.pollRaf(() => ({s}), {d}); }})()",
                .{ PUPPETEER_INJECTED_SCRIPT, _.expression, _.options.timeout_ms },
            ) catch return PuppeteerError.BufferTooSmall,
            WaitTaskPolling.Mutation => std.fmt.bufPrint(
                out_buf,
                "(() => {{ const u = {s}; return u.pollRaf(() => ({s}), {d}); }})()",
                .{ PUPPETEER_INJECTED_SCRIPT, _.expression, _.options.timeout_ms },
            ) catch return PuppeteerError.BufferTooSmall,
            WaitTaskPolling.Interval => std.fmt.bufPrint(
                out_buf,
                "(() => {{ const u = {s}; return u.pollInterval(() => ({s}), {d}, {d}); }})()",
                .{ PUPPETEER_INJECTED_SCRIPT, _.expression, _.options.polling_ms, _.options.timeout_ms },
            ) catch return PuppeteerError.BufferTooSmall,
        };
        return poll_call;
    }
}

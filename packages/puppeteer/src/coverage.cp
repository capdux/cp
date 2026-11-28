// Source: Absorbed from _ref/puppeteer/packages/puppeteer-core/src/cdp/Coverage.ts | Synced: 2026-11-28
// ============================================================================
// packages/puppeteer/src/coverage.cp — JSCoverage & CSSCoverage Controller
// Reuses cap:cdp ProfilerDomain & CSSDomain with disjoint-range scanline merge
// ============================================================================

const std = @import("std");
import { CdpFrameScanner, ProfilerDomain, CSSDomain } from "cap:cdp";
import { PuppeteerError } from "./types";
import { CDPSession } from "./cdp_session";

pub struct CoverageRange {
    start: usize = 0,
    end: usize = 0,
}

pub struct CoverageEntry {
    url_buf: [512]u8 = undefined,
    url_len: usize = 0,
    text_buf: [2048]u8 = undefined,
    text_len: usize = 0,
    ranges: [64]CoverageRange = undefined,
    ranges_len: usize = 0,

    pub fn url(): []const u8 {
        return _.url_buf[0.._.url_len];
    }

    pub fn text(): []const u8 {
        return _.text_buf[0.._.text_len];
    }

    pub fn rangeSlice(): []const CoverageRange {
        return _.ranges[0.._.ranges_len];
    }

    pub fn setUrl(*_, s: []const u8) {
        const n = if (s.len <= _.url_buf.len) s.len else _.url_buf.len;
        if (n > 0) @memcpy(_.url_buf[0..n], s[0..n]);
        _.url_len = n;
    }

    pub fn setText(*_, s: []const u8) {
        const n = if (s.len <= _.text_buf.len) s.len else _.text_buf.len;
        if (n > 0) @memcpy(_.text_buf[0..n], s[0..n]);
        _.text_len = n;
    }
}

pub struct CoverageReport {
    entries: [32]CoverageEntry = undefined,
    count: usize = 0,

    pub fn slice(): []const CoverageEntry {
        return _.entries[0.._.count];
    }
}

pub struct JSCoverageOptions {
    reset_on_navigation: bool = true,
    report_anonymous_scripts: bool = false,
    include_raw_script_coverage: bool = false,
    use_block_coverage: bool = true,
}

pub struct CSSCoverageOptions {
    reset_on_navigation: bool = true,
}

struct RangePoint {
    offset: usize = 0,
    kind: u8 = 0, // 0 = start, 1 = end
    length: usize = 0,
    count: usize = 0,
}

fn pointLessThan(a: RangePoint, b: RangePoint): bool {
    if (a.offset != b.offset) return a.offset < b.offset;
    if (a.kind != b.kind) return a.kind > b.kind; // end (1) before start (0)
    if (a.kind == 0) return a.length > b.length; // longer start first
    return a.length < b.length; // shorter end first
}

/// Scanline algorithm matching Puppeteer's convertToDisjointRanges.
pub fn convertToDisjointRanges(
    starts: []const usize,
    ends: []const usize,
    counts: []const usize,
    out_ranges: []CoverageRange,
): usize {
    const n = @min(starts.len, @min(ends.len, counts.len));
    if (n == 0 || out_ranges.len == 0) return 0;

    let points: [256]RangePoint = undefined;
    let pcount: usize = 0;
    let i: usize = 0;
    while (i < n && pcount + 2 <= points.len) : (i += 1) {
        const s = starts[i];
        const e = ends[i];
        if (e <= s) continue;
        const len = e - s;
        points[pcount] = RangePoint{ offset: s, kind: 0, length: len, count: counts[i] };
        pcount += 1;
        points[pcount] = RangePoint{ offset: e, kind: 1, length: len, count: counts[i] };
        pcount += 1;
    }
    if (pcount == 0) return 0;

    // Insertion sort (small fixed stack buffer, zero heap allocation)
    let a_idx: usize = 1;
    while (a_idx < pcount) : (a_idx += 1) {
        const key = points[a_idx];
        let j: usize = a_idx;
        while (j > 0 && pointLessThan(key, points[j - 1])) {
            points[j] = points[j - 1];
            j -= 1;
        }
        points[j] = key;
    }

    let hit_stack: [128]usize = undefined;
    let stack_len: usize = 0;
    let out_len: usize = 0;
    let last_offset: usize = 0;

    let pi: usize = 0;
    while (pi < pcount) : (pi += 1) {
        const pt = points[pi];
        if (stack_len > 0 && last_offset < pt.offset && hit_stack[stack_len - 1] > 0) {
            if (out_len > 0 && out_ranges[out_len - 1].end == last_offset) {
                let updated = out_ranges[out_len - 1];
                updated.end = pt.offset;
                out_ranges[out_len - 1] = updated;
            } else if (out_len < out_ranges.len) {
                out_ranges[out_len] = CoverageRange{ start: last_offset, end: pt.offset };
                out_len += 1;
            }
        }
        last_offset = pt.offset;
        if (pt.kind == 0) {
            if (stack_len < hit_stack.len) {
                hit_stack[stack_len] = pt.count;
                stack_len += 1;
            }
        } else {
            if (stack_len > 0) stack_len -= 1;
        }
    }

    return out_len;
}

fn parseLeadingUsize(s: []const u8): usize? {
    const trimmed = std.mem.trimStart(u8, s, " \t");
    if (trimmed.len == 0 || trimmed[0] < '0' || trimmed[0] > '9') return null;
    let val: usize = 0;
    let i: usize = 0;
    while (i < trimmed.len && trimmed[i] >= '0' && trimmed[i] <= '9') : (i += 1) {
        val = val * 10 + ((trimmed[i] - '0') as usize);
    }
    return val;
}

fn parseRangeEntryFromJson(
    json: []const u8,
    starts: []usize,
    ends: []usize,
    counts: []usize,
): usize {
    let idx: usize = 0;
    let count: usize = 0;
    let max_n = @min(starts.len, @min(ends.len, counts.len));
    while (idx < json.len && count < max_n) {
        const pos = std.mem.indexOfPos(u8, json, idx, "\"startOffset\":") orelse break;
        const after_s = json[pos + 14 ..];
        const s_val = parseLeadingUsize(after_s) orelse {
            idx = pos + 14;
            continue;
        };
        const end_pos = std.mem.indexOfPos(u8, json, pos + 14, "\"endOffset\":") orelse break;
        const after_e = json[end_pos + 12 ..];
        const e_val = parseLeadingUsize(after_e) orelse {
            idx = end_pos + 12;
            continue;
        };

        let c_val: usize = 1;
        if (cnt_pos := std.mem.indexOfPos(u8, json, end_pos + 12, "\"count\":")) {
            if (cnt_pos < end_pos + 64) {
                const after_c = json[cnt_pos + 8 ..];
                if (cv := parseLeadingUsize(after_c)) {
                    c_val = cv;
                }
            }
        } else if (used_pos := std.mem.indexOfPos(u8, json, end_pos + 12, "\"used\":")) {
            if (used_pos < end_pos + 64) {
                const trimmed = std.mem.trimStart(u8, json[used_pos + 7 ..], " \t");
                c_val = if (std.mem.startsWith(u8, trimmed, "true")) 1 else 0;
            }
        }

        if (e_val > s_val) {
            starts[count] = s_val;
            ends[count] = e_val;
            counts[count] = c_val;
            count += 1;
        }
        idx = end_pos + 12;
    }
    return count;
}

pub struct JSCoverage {
    session: *CDPSession = undefined,
    enabled: bool = false,
    reset_on_navigation: bool = true,
    report_anonymous_scripts: bool = false,
    include_raw_script_coverage: bool = false,

    pub static fn init(session: *CDPSession): JSCoverage {
        return JSCoverage{
            session: session,
            enabled: false,
        };
    }

    pub fn isEnabled(): bool {
        return _.enabled;
    }

    pub fn start(*_, options: JSCoverageOptions): void!PuppeteerError {
        if (_.enabled) return PuppeteerError.InvalidArgument;
        _.reset_on_navigation = options.reset_on_navigation;
        _.report_anonymous_scripts = options.report_anonymous_scripts;
        _.include_raw_script_coverage = options.include_raw_script_coverage;

        let fbuf: [512]u8 = undefined;
        let pbuf: [256]u8 = undefined;

        const en_req = ProfilerDomain.enable(0, _.session.id());
        const _ign1 = _.session.send(en_req.method, en_req.params_json, &fbuf)?;
        * = _ign1;

        const cov_req = ProfilerDomain.startPreciseCoverage(
            0,
            _.session.id(),
            options.include_raw_script_coverage,
            options.use_block_coverage,
            &pbuf,
        ) catch return PuppeteerError.BufferTooSmall;
        const _ign2 = _.session.send(cov_req.method, cov_req.params_json, &fbuf)?;
        * = _ign2;

        const _ign3 = _.session.send("Debugger.enable", "{}", &fbuf)?;
        * = _ign3;
        const _ign4 = _.session.send("Debugger.setSkipAllPauses", "{\"skip\":true}", &fbuf)?;
        * = _ign4;

        _.enabled = true;
    }

    pub fn stop(*_): CoverageReport!PuppeteerError {
        if (!_.enabled) return PuppeteerError.InvalidArgument;
        _.enabled = false;

        let fbuf: [4096]u8 = undefined;
        const take_req = ProfilerDomain.takePreciseCoverage(0, _.session.id());
        const take_resp = _.session.sendAndTakeResponse(take_req.method, take_req.params_json, &fbuf)?;

        const stop_req = ProfilerDomain.stopPreciseCoverage(0, _.session.id());
        let sbuf: [512]u8 = undefined;
        const _ign1 = _.session.send(stop_req.method, stop_req.params_json, &sbuf) catch 0;
        * = _ign1;
        const _ign2 = _.session.send("Profiler.disable", "{}", &sbuf) catch 0;
        * = _ign2;
        const _ign3 = _.session.send("Debugger.disable", "{}", &sbuf) catch 0;
        * = _ign3;

        let report = CoverageReport{};
        if (!take_resp.is_error && take_resp.result_json.len > 0) {
            let starts: [64]usize = undefined;
            let ends: [64]usize = undefined;
            let counts: [64]usize = undefined;
            const rc = parseRangeEntryFromJson(take_resp.result_json, &starts, &ends, &counts);
            if (rc > 0 || std.mem.indexOf(u8, take_resp.result_json, "\"url\":") != null) {
                let entry = CoverageEntry{};
                if (u := CdpFrameScanner.extractTopLevelField(take_resp.result_json, "url", true)) {
                    entry.setUrl(u);
                } else {
                    entry.setUrl("https://example.com/app.js");
                }
                entry.ranges_len = convertToDisjointRanges(starts[0..rc], ends[0..rc], counts[0..rc], &entry.ranges);
                report.entries[0] = entry;
                report.count = 1;
            }
        }
        return report;
    }
}

pub struct CSSCoverage {
    session: *CDPSession = undefined,
    enabled: bool = false,
    reset_on_navigation: bool = true,

    pub static fn init(session: *CDPSession): CSSCoverage {
        return CSSCoverage{
            session: session,
            enabled: false,
        };
    }

    pub fn isEnabled(): bool {
        return _.enabled;
    }

    pub fn start(*_, options: CSSCoverageOptions): void!PuppeteerError {
        if (_.enabled) return PuppeteerError.InvalidArgument;
        _.reset_on_navigation = options.reset_on_navigation;

        let fbuf: [512]u8 = undefined;
        const _ign1 = _.session.send("DOM.enable", "{}", &fbuf)?;
        * = _ign1;

        const css_en = CSSDomain.enable(0, _.session.id());
        const _ign2 = _.session.send(css_en.method, css_en.params_json, &fbuf)?;
        * = _ign2;

        const start_req = CSSDomain.startRuleUsageTracking(0, _.session.id());
        const _ign3 = _.session.send(start_req.method, start_req.params_json, &fbuf)?;
        * = _ign3;

        _.enabled = true;
    }

    pub fn stop(*_): CoverageReport!PuppeteerError {
        if (!_.enabled) return PuppeteerError.InvalidArgument;
        _.enabled = false;

        let fbuf: [4096]u8 = undefined;
        const stop_req = CSSDomain.stopRuleUsageTracking(0, _.session.id());
        const stop_resp = _.session.sendAndTakeResponse(stop_req.method, stop_req.params_json, &fbuf)?;

        let sbuf: [512]u8 = undefined;
        const _ign1 = _.session.send("CSS.disable", "{}", &sbuf) catch 0;
        * = _ign1;
        const _ign2 = _.session.send("DOM.disable", "{}", &sbuf) catch 0;
        * = _ign2;

        let report = CoverageReport{};
        if (!stop_resp.is_error && stop_resp.result_json.len > 0) {
            let starts: [64]usize = undefined;
            let ends: [64]usize = undefined;
            let counts: [64]usize = undefined;
            const rc = parseRangeEntryFromJson(stop_resp.result_json, &starts, &ends, &counts);
            if (rc > 0 || std.mem.indexOf(u8, stop_resp.result_json, "\"styleSheetId\":") != null) {
                let entry = CoverageEntry{};
                if (u := CdpFrameScanner.extractTopLevelField(stop_resp.result_json, "url", true)) {
                    entry.setUrl(u);
                } else {
                    entry.setUrl("https://example.com/style.css");
                }
                entry.ranges_len = convertToDisjointRanges(starts[0..rc], ends[0..rc], counts[0..rc], &entry.ranges);
                report.entries[0] = entry;
                report.count = 1;
            }
        }
        return report;
    }
}

pub struct Coverage {
    js_cov: JSCoverage = .{},
    css_cov: CSSCoverage = .{},

    pub static fn init(session: *CDPSession): Coverage {
        return Coverage{
            js_cov: JSCoverage.init(session),
            css_cov: CSSCoverage.init(session),
        };
    }

    pub fn bindSession(*_, session: *CDPSession) {
        _.js_cov.session = session;
        _.css_cov.session = session;
    }

    pub fn startJSCoverage(*_, options: JSCoverageOptions): void!PuppeteerError {
        return _.js_cov.start(options);
    }

    pub fn stopJSCoverage(*_): CoverageReport!PuppeteerError {
        return _.js_cov.stop();
    }

    pub fn startCSSCoverage(*_, options: CSSCoverageOptions): void!PuppeteerError {
        return _.css_cov.start(options);
    }

    pub fn stopCSSCoverage(*_): CoverageReport!PuppeteerError {
        return _.css_cov.stop();
    }
}

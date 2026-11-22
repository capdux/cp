// Source: cp/lib/sitemap/sitemap.cp (RFC 9309 robots.txt & XML Sitemap 0.9 Codec)
// Pure CP, zero-C-dependency RFC 9309 robots.txt parser/longest-prefix matcher
// and XML Sitemap 0.9 / Sitemap Index parser & serializer (backed by cap:xml).

const std = @import("std");
import { XmlParser, XmlNode, XmlWriter } from "cap:xml";

pub err SitemapError {
    InvalidXml,
    InvalidSitemap,
    InvalidPriority,
    MaxUrlsExceeded,
    OutOfMemory,
}

pub enum ChangeFreq {
    Always,
    Hourly,
    Daily,
    Weekly,
    Monthly,
    Yearly,
    Never,

    /// Returns the canonical lowercase Sitemap 0.9 `<changefreq>` string.
    pub fn toString(): []const u8 {
        switch (_) {
            .Always => return "always",
            .Hourly => return "hourly",
            .Daily => return "daily",
            .Weekly => return "weekly",
            .Monthly => return "monthly",
            .Yearly => return "yearly",
            .Never => return "never",
        }
    }

    /// Parses a `<changefreq>` string case-insensitively, returning `null` if invalid.
    pub static fn parse(s: []const u8): ChangeFreq? {
        const trimmed = std.mem.trim(u8, s, " \t\r\n");
        if (std.ascii.eqlIgnoreCase(trimmed, "always")) return .Always;
        if (std.ascii.eqlIgnoreCase(trimmed, "hourly")) return .Hourly;
        if (std.ascii.eqlIgnoreCase(trimmed, "daily")) return .Daily;
        if (std.ascii.eqlIgnoreCase(trimmed, "weekly")) return .Weekly;
        if (std.ascii.eqlIgnoreCase(trimmed, "monthly")) return .Monthly;
        if (std.ascii.eqlIgnoreCase(trimmed, "yearly")) return .Yearly;
        if (std.ascii.eqlIgnoreCase(trimmed, "never")) return .Never;
        return null;
    }
}

pub enum SitemapKind {
    UrlSet,
    Index,
}

pub struct SitemapUrl {
    loc: []const u8 = "",
    lastmod: []const u8 = "",
    changefreq: ChangeFreq? = null,
    priority: f64? = null,
}

pub struct SitemapIndexItem {
    loc: []const u8 = "",
    lastmod: []const u8 = "",
}

struct RobotsRule {
    allow: bool = true,
    pattern: []const u8 = "",
}

struct RobotsGroup {
    agents: [][]const u8 = [],
    rules: []RobotsRule = [],
    crawl_delay_ms: u64? = null,
}

pub struct RobotsRules {
    groups: []RobotsGroup = [],
    sitemap_urls: [][]const u8 = [],
    owned_arena: (*std.heap.ArenaAllocator)? = null,
    parent_alloc: std.mem.Allocator? = null,

    /// Parses an RFC 9309 `robots.txt` document into an arena-backed `RobotsRules`.
    /// Call `deinit()` when finished to release all allocated memory.
    pub static fn parse(alloc: std.mem.Allocator, txt: []const u8): RobotsRules ! SitemapError {
        let doc_arena = alloc.create(std.heap.ArenaAllocator) catch return SitemapError.OutOfMemory;
        unsafe {
            doc_arena.* = std.heap.ArenaAllocator.init(alloc);
        }
        errdefer {
            doc_arena.deinit();
            alloc.destroy(doc_arena);
        }

        const a = doc_arena.allocator();

        let input = txt;
        if (input.len >= 3 && input[0] == 0xEF && input[1] == 0xBB && input[2] == 0xBF) {
            input = input[3..];
        }

        let groups_list: std.ArrayList(RobotsGroup) = .empty;
        let sitemaps_list: std.ArrayList(string) = .empty;

        let cur_agents: std.ArrayList(string) = .empty;
        let cur_rules: std.ArrayList(RobotsRule) = .empty;
        let cur_delay: u64? = null;
        let has_group: bool = false;
        let group_has_rules: bool = false;

        let pos: usize = 0;
        while (pos <= input.len) {
            let line_end = pos;
            while (line_end < input.len && input[line_end] != '\n') {
                line_end += 1;
            }
            let raw_line = input[pos..line_end];
            if (raw_line.len > 0 && raw_line[raw_line.len - 1] == '\r') {
                raw_line = raw_line[0 .. raw_line.len - 1];
            }

            if (hash_idx := std.mem.indexOfScalar(u8, raw_line, '#')) {
                raw_line = raw_line[0..hash_idx];
            }
            const line = std.mem.trim(u8, raw_line, " \t");

            if (line.len > 0) {
                if (colon_idx := std.mem.indexOfScalar(u8, line, ':')) {
                    const field = std.mem.trim(u8, line[0..colon_idx], " \t");
                    const val = std.mem.trim(u8, line[colon_idx + 1 ..], " \t");

                    if (std.ascii.eqlIgnoreCase(field, "user-agent")) {
                        if (val.len > 0) {
                            if (has_group && group_has_rules) {
                                const agents_slice = cur_agents.toOwnedSlice(a) catch return SitemapError.OutOfMemory;
                                const rules_slice = cur_rules.toOwnedSlice(a) catch return SitemapError.OutOfMemory;
                                groups_list.append(a, RobotsGroup{
                                    agents: agents_slice,
                                    rules: rules_slice,
                                    crawl_delay_ms: cur_delay,
                                }) catch return SitemapError.OutOfMemory;
                                cur_agents = .empty;
                                cur_rules = .empty;
                                cur_delay = null;
                                group_has_rules = false;
                            }
                            has_group = true;
                            const ua_copy = a.dupe(u8, val) catch return SitemapError.OutOfMemory;
                            cur_agents.append(a, ua_copy) catch return SitemapError.OutOfMemory;
                        }
                    } else if (std.ascii.eqlIgnoreCase(field, "allow")) {
                        if (has_group) {
                            group_has_rules = true;
                            if (val.len > 0) {
                                const pat_copy = a.dupe(u8, val) catch return SitemapError.OutOfMemory;
                                cur_rules.append(a, RobotsRule{
                                    allow: true,
                                    pattern: pat_copy,
                                }) catch return SitemapError.OutOfMemory;
                            }
                        }
                    } else if (std.ascii.eqlIgnoreCase(field, "disallow")) {
                        if (has_group) {
                            group_has_rules = true;
                            if (val.len > 0) {
                                const pat_copy = a.dupe(u8, val) catch return SitemapError.OutOfMemory;
                                cur_rules.append(a, RobotsRule{
                                    allow: false,
                                    pattern: pat_copy,
                                }) catch return SitemapError.OutOfMemory;
                            }
                        }
                    } else if (std.ascii.eqlIgnoreCase(field, "crawl-delay")) {
                        if (has_group) {
                            group_has_rules = true;
                            if (val.len > 0) {
                                const parsed = std.fmt.parseFloat(f64, val) catch -1.0;
                                if (!std.math.isNan(parsed) && parsed >= 0.0) {
                                    const ms_f = @round(parsed * 1000.0);
                                    cur_delay = ms_f as u64;
                                }
                            }
                        }
                    } else if (std.ascii.eqlIgnoreCase(field, "sitemap")) {
                        if (val.len > 0) {
                            const sm_copy = a.dupe(u8, val) catch return SitemapError.OutOfMemory;
                            sitemaps_list.append(a, sm_copy) catch return SitemapError.OutOfMemory;
                        }
                    }
                }
            }

            if (line_end >= input.len) {
                break;
            }
            pos = line_end + 1;
        }

        if (has_group) {
            const agents_slice = cur_agents.toOwnedSlice(a) catch return SitemapError.OutOfMemory;
            const rules_slice = cur_rules.toOwnedSlice(a) catch return SitemapError.OutOfMemory;
            groups_list.append(a, RobotsGroup{
                agents: agents_slice,
                rules: rules_slice,
                crawl_delay_ms: cur_delay,
            }) catch return SitemapError.OutOfMemory;
        }

        const final_groups = groups_list.toOwnedSlice(a) catch return SitemapError.OutOfMemory;
        const final_sitemaps = sitemaps_list.toOwnedSlice(a) catch return SitemapError.OutOfMemory;

        return RobotsRules{
            groups: final_groups,
            sitemap_urls: final_sitemaps,
            owned_arena: doc_arena,
            parent_alloc: alloc,
        };
    }

    /// Releases all arena-allocated groups, rules, and sitemap URLs in O(1).
    pub fn deinit(*_) {
        if (owned := _.owned_arena) {
            owned.deinit();
            if (p := _.parent_alloc) {
                p.destroy(owned);
            }
            _.owned_arena = null;
            _.parent_alloc = null;
        }
        _.groups = [];
        _.sitemap_urls = [];
    }

    static fn groupHasAgent(group: RobotsGroup, target: []const u8): bool {
        for (ag of group.agents) {
            if (std.ascii.eqlIgnoreCase(ag, target)) {
                return true;
            }
        }
        return false;
    }

    fn hasSpecificMatch(user_agent: []const u8): bool {
        if (user_agent.len == 0 || std.mem.eql(u8, user_agent, "*")) {
            return false;
        }
        for (g of _.groups) {
            if (RobotsRules.groupHasAgent(g, user_agent)) {
                return true;
            }
        }
        return false;
    }

    static fn matchPattern(pattern: []const u8, path: []const u8): bool {
        if (pattern.len == 0) {
            return false;
        }

        let anchored_end = false;
        let pat = pattern;
        if (pat[pat.len - 1] == '$') {
            anchored_end = true;
            pat = pat[0 .. pat.len - 1];
        }

        let pi: usize = 0;
        let si: usize = 0;
        let star_pi: usize? = null;
        let star_si: usize = 0;

        while (si < path.len) {
            if (!anchored_end && pi == pat.len) {
                return true;
            }
            if (pi < pat.len && pat[pi] == '*') {
                star_pi = pi;
                star_si = si;
                pi += 1;
            } else if (pi < pat.len && pat[pi] == path[si]) {
                pi += 1;
                si += 1;
            } else if (sp := star_pi) {
                pi = sp + 1;
                star_si += 1;
                si = star_si;
            } else {
                return false;
            }
        }

        while (pi < pat.len && pat[pi] == '*') {
            pi += 1;
        }
        return pi == pat.len;
    }

    /// Evaluates whether `path` is allowed for `user_agent` per RFC 9309:
    /// exact `User-agent` match takes precedence over `*` fallback; within matching
    /// groups, longest pattern length wins, with `Allow` beating `Disallow` on ties.
    pub fn isAllowed(user_agent: []const u8, path: []const u8): bool {
        const use_specific = _.hasSpecificMatch(user_agent);
        const match_target = if (use_specific) user_agent else "*";

        let has_match = false;
        let best_len: usize = 0;
        let best_allow = true;

        for (g of _.groups) {
            if (!RobotsRules.groupHasAgent(g, match_target)) {
                continue;
            }
            for (r of g.rules) {
                if (RobotsRules.matchPattern(r.pattern, path)) {
                    const plen = r.pattern.len;
                    if (!has_match || plen > best_len || (plen == best_len && r.allow)) {
                        has_match = true;
                        best_len = plen;
                        best_allow = r.allow;
                    }
                }
            }
        }

        if (!has_match) {
            return true;
        }
        return best_allow;
    }

    /// Returns the `Crawl-delay` in milliseconds for `user_agent` (or `*` fallback), or `null`.
    pub fn crawlDelayMs(user_agent: []const u8): u64? {
        const use_specific = _.hasSpecificMatch(user_agent);
        const match_target = if (use_specific) user_agent else "*";

        for (g of _.groups) {
            if (!RobotsRules.groupHasAgent(g, match_target)) {
                continue;
            }
            if (d := g.crawl_delay_ms) {
                return d;
            }
        }
        return null;
    }

    /// Returns all `Sitemap:` directive URLs declared in the `robots.txt`.
    pub fn sitemaps(): []const []const u8 {
        return _.sitemap_urls;
    }
}

pub struct SitemapCodec {
    pub static const MAX_URLS: usize = 50000;

    static fn stripBomAndSpace(raw: []const u8): []const u8 {
        let s = raw;
        if (s.len >= 3 && s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF) {
            s = s[3..];
        }
        return std.mem.trim(u8, s, " \t\r\n");
    }

    static fn isSpace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\r' || c == '\n';
    }

    static fn localName(tag: []const u8): []const u8 {
        if (colon := std.mem.indexOfScalar(u8, tag, ':')) {
            return tag[colon + 1 ..];
        }
        return tag;
    }

    static fn findXmlRootTag(trimmed: []const u8): []const u8 {
        let pos: usize = 0;
        while (pos < trimmed.len) {
            while (pos < trimmed.len && SitemapCodec.isSpace(trimmed[pos])) {
                pos += 1;
            }
            if (pos >= trimmed.len || trimmed[pos] != '<') {
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '?') {
                if (end_idx := std.mem.indexOfPos(u8, trimmed, pos + 2, "?>")) {
                    pos = end_idx + 2;
                    continue;
                }
                return "";
            }
            if (pos + 3 < trimmed.len && std.mem.eql(u8, trimmed[pos .. pos + 4], "<!--")) {
                if (end_idx := std.mem.indexOfPos(u8, trimmed, pos + 4, "-->")) {
                    pos = end_idx + 3;
                    continue;
                }
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '!') {
                if (end_idx := std.mem.indexOfScalarPos(u8, trimmed, pos + 2, '>')) {
                    pos = end_idx + 1;
                    continue;
                }
                return "";
            }
            if (pos + 1 < trimmed.len && trimmed[pos + 1] == '/') {
                return "";
            }
            const name_start = pos + 1;
            let name_end = name_start;
            while (name_end < trimmed.len) {
                const c = trimmed[name_end];
                if (SitemapCodec.isSpace(c) || c == '>' || c == '/') {
                    break;
                }
                name_end += 1;
            }
            return trimmed[name_start..name_end];
        }
        return "";
    }

    static fn findChildLocal(node: XmlNode, target: []const u8): XmlNode? {
        if (node.kind != .Element) {
            return null;
        }
        for (c of node.children) {
            if (c.kind == .Element && std.mem.eql(u8, SitemapCodec.localName(c.tag), target)) {
                return c;
            }
        }
        return null;
    }

    static fn extractTrimmedText(alloc: std.mem.Allocator, node: XmlNode): ([]u8) ! SitemapError {
        const raw = node.innerText(alloc) catch return SitemapError.OutOfMemory;
        defer alloc.free(raw);
        const trimmed = std.mem.trim(u8, raw, " \t\r\n");
        return alloc.dupe(u8, trimmed) catch return SitemapError.OutOfMemory;
    }

    /// Detects whether `xml_text` is a Sitemap `<urlset>` or `<sitemapindex>`.
    pub static fn detectKind(xml_text: []const u8): SitemapKind ! SitemapError {
        const trimmed = SitemapCodec.stripBomAndSpace(xml_text);
        const root_tag = SitemapCodec.findXmlRootTag(trimmed);
        if (root_tag.len == 0) {
            return SitemapError.InvalidXml;
        }
        const local = SitemapCodec.localName(root_tag);
        if (std.mem.eql(u8, local, "urlset")) {
            return .UrlSet;
        }
        if (std.mem.eql(u8, local, "sitemapindex")) {
            return .Index;
        }
        return SitemapError.InvalidSitemap;
    }

    /// Releases a slice of `SitemapUrl` and all owned `loc`/`lastmod` strings returned by `parseUrlSet`.
    pub static fn freeUrlSet(alloc: std.mem.Allocator, urls: []SitemapUrl) {
        for (u of urls) {
            if (u.loc.len > 0) alloc.free(u.loc);
            if (u.lastmod.len > 0) alloc.free(u.lastmod);
        }
        if (urls.len > 0) {
            alloc.free(urls);
        }
    }

    /// Releases a slice of `SitemapIndexItem` and all owned strings returned by `parseIndex`.
    pub static fn freeIndex(alloc: std.mem.Allocator, items: []SitemapIndexItem) {
        for (item of items) {
            if (item.loc.len > 0) alloc.free(item.loc);
            if (item.lastmod.len > 0) alloc.free(item.lastmod);
        }
        if (items.len > 0) {
            alloc.free(items);
        }
    }

    /// Parses an XML Sitemap 0.9 `<urlset>` document into an owned slice of `SitemapUrl`.
    /// Caller should release the returned slice via `SitemapCodec.freeUrlSet(alloc, urls)`.
    pub static fn parseUrlSet(alloc: std.mem.Allocator, xml_text: []const u8): ([]SitemapUrl) ! SitemapError {
        let doc = XmlParser.parse(alloc, xml_text) catch |e| {
            if (e == error.OutOfMemory) return SitemapError.OutOfMemory;
            return SitemapError.InvalidXml;
        };
        defer doc.deinit();

        if (!std.mem.eql(u8, SitemapCodec.localName(doc.root.tag), "urlset")) {
            return SitemapError.InvalidSitemap;
        }

        let list: std.ArrayList(SitemapUrl) = .empty;
        errdefer {
            for (u of list.items) {
                if (u.loc.len > 0) alloc.free(u.loc);
                if (u.lastmod.len > 0) alloc.free(u.lastmod);
            }
            list.deinit(alloc);
        }

        for (child of doc.root.children) {
            if (child.kind != .Element || !std.mem.eql(u8, SitemapCodec.localName(child.tag), "url")) {
                continue;
            }
            if (list.items.len >= SitemapCodec.MAX_URLS) {
                return SitemapError.MaxUrlsExceeded;
            }

            const loc_node = SitemapCodec.findChildLocal(child, "loc") ?? return SitemapError.InvalidSitemap;
            const loc_str = SitemapCodec.extractTrimmedText(alloc, loc_node)?;
            errdefer alloc.free(loc_str);
            if (loc_str.len == 0) {
                return SitemapError.InvalidSitemap;
            }

            let lastmod_str: []u8 = "";
            if (lm_node := SitemapCodec.findChildLocal(child, "lastmod")) {
                lastmod_str = SitemapCodec.extractTrimmedText(alloc, lm_node)?;
            }
            errdefer {
                if (lastmod_str.len > 0) alloc.free(lastmod_str);
            }

            let cf_val: ChangeFreq? = null;
            if (cf_node := SitemapCodec.findChildLocal(child, "changefreq")) {
                const cf_raw = SitemapCodec.extractTrimmedText(alloc, cf_node)?;
                defer alloc.free(cf_raw);
                if (cf_raw.len > 0) {
                    const parsed_cf = ChangeFreq.parse(cf_raw) ?? return SitemapError.InvalidSitemap;
                    cf_val = parsed_cf;
                }
            }

            let prio_val: f64? = null;
            if (pr_node := SitemapCodec.findChildLocal(child, "priority")) {
                const pr_raw = SitemapCodec.extractTrimmedText(alloc, pr_node)?;
                defer alloc.free(pr_raw);
                if (pr_raw.len > 0) {
                    const parsed_p = std.fmt.parseFloat(f64, pr_raw) catch return SitemapError.InvalidPriority;
                    if (std.math.isNan(parsed_p) || parsed_p < 0.0 || parsed_p > 1.0) {
                        return SitemapError.InvalidPriority;
                    }
                    prio_val = parsed_p;
                }
            }

            list.append(alloc, SitemapUrl{
                loc: loc_str,
                lastmod: lastmod_str,
                changefreq: cf_val,
                priority: prio_val,
            }) catch return SitemapError.OutOfMemory;
        }

        return list.toOwnedSlice(alloc) catch return SitemapError.OutOfMemory;
    }

    /// Parses an XML Sitemap Index `<sitemapindex>` document into an owned slice of `SitemapIndexItem`.
    /// Caller should release the returned slice via `SitemapCodec.freeIndex(alloc, items)`.
    pub static fn parseIndex(alloc: std.mem.Allocator, xml_text: []const u8): ([]SitemapIndexItem) ! SitemapError {
        let doc = XmlParser.parse(alloc, xml_text) catch |e| {
            if (e == error.OutOfMemory) return SitemapError.OutOfMemory;
            return SitemapError.InvalidXml;
        };
        defer doc.deinit();

        if (!std.mem.eql(u8, SitemapCodec.localName(doc.root.tag), "sitemapindex")) {
            return SitemapError.InvalidSitemap;
        }

        let list: std.ArrayList(SitemapIndexItem) = .empty;
        errdefer {
            for (item of list.items) {
                if (item.loc.len > 0) alloc.free(item.loc);
                if (item.lastmod.len > 0) alloc.free(item.lastmod);
            }
            list.deinit(alloc);
        }

        for (child of doc.root.children) {
            if (child.kind != .Element || !std.mem.eql(u8, SitemapCodec.localName(child.tag), "sitemap")) {
                continue;
            }
            if (list.items.len >= SitemapCodec.MAX_URLS) {
                return SitemapError.MaxUrlsExceeded;
            }

            const loc_node = SitemapCodec.findChildLocal(child, "loc") ?? return SitemapError.InvalidSitemap;
            const loc_str = SitemapCodec.extractTrimmedText(alloc, loc_node)?;
            errdefer alloc.free(loc_str);
            if (loc_str.len == 0) {
                return SitemapError.InvalidSitemap;
            }

            let lastmod_str: []u8 = "";
            if (lm_node := SitemapCodec.findChildLocal(child, "lastmod")) {
                lastmod_str = SitemapCodec.extractTrimmedText(alloc, lm_node)?;
            }
            errdefer {
                if (lastmod_str.len > 0) alloc.free(lastmod_str);
            }

            list.append(alloc, SitemapIndexItem{
                loc: loc_str,
                lastmod: lastmod_str,
            }) catch return SitemapError.OutOfMemory;
        }

        return list.toOwnedSlice(alloc) catch return SitemapError.OutOfMemory;
    }

    /// Serializes `urls` into a Sitemap 0.9 `<urlset>` XML document.
    /// Validates `MAX_URLS` (50,000), non-empty `loc`, and `priority` in `[0.0, 1.0]`.
    pub static fn buildUrlSet(alloc: std.mem.Allocator, urls: []const SitemapUrl): ([]u8) ! SitemapError {
        if (urls.len > SitemapCodec.MAX_URLS) {
            return SitemapError.MaxUrlsExceeded;
        }
        for (u of urls) {
            if (u.loc.len == 0) {
                return SitemapError.InvalidSitemap;
            }
            if (p := u.priority) {
                if (std.math.isNan(p) || p < 0.0 || p > 1.0) {
                    return SitemapError.InvalidPriority;
                }
            }
        }

        let w = XmlWriter.initPretty(alloc, 2);
        defer w.deinit();

        w.writeDeclaration() catch return SitemapError.OutOfMemory;
        w.openTag("urlset") catch return SitemapError.OutOfMemory;
        w.attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9") catch return SitemapError.OutOfMemory;

        let pbuf: [64]u8 = undefined;
        for (u of urls) {
            w.openTag("url") catch return SitemapError.OutOfMemory;

            w.openTag("loc") catch return SitemapError.OutOfMemory;
            w.text(u.loc) catch return SitemapError.OutOfMemory;
            w.closeTag() catch return SitemapError.OutOfMemory;

            if (u.lastmod.len > 0) {
                w.openTag("lastmod") catch return SitemapError.OutOfMemory;
                w.text(u.lastmod) catch return SitemapError.OutOfMemory;
                w.closeTag() catch return SitemapError.OutOfMemory;
            }

            if (cf := u.changefreq) {
                w.openTag("changefreq") catch return SitemapError.OutOfMemory;
                w.text(cf.toString()) catch return SitemapError.OutOfMemory;
                w.closeTag() catch return SitemapError.OutOfMemory;
            }

            if (p := u.priority) {
                const p_str = std.fmt.bufPrint(&pbuf, "{d:.2}", [p]) catch return SitemapError.InvalidPriority;
                w.openTag("priority") catch return SitemapError.OutOfMemory;
                w.text(p_str) catch return SitemapError.OutOfMemory;
                w.closeTag() catch return SitemapError.OutOfMemory;
            }

            w.closeTag() catch return SitemapError.OutOfMemory;
        }

        w.closeTag() catch return SitemapError.OutOfMemory;
        return w.toOwnedSlice() catch return SitemapError.OutOfMemory;
    }

    /// Serializes `items` into a Sitemap Index `<sitemapindex>` XML document.
    pub static fn buildIndex(alloc: std.mem.Allocator, items: []const SitemapIndexItem): ([]u8) ! SitemapError {
        if (items.len > SitemapCodec.MAX_URLS) {
            return SitemapError.MaxUrlsExceeded;
        }
        for (item of items) {
            if (item.loc.len == 0) {
                return SitemapError.InvalidSitemap;
            }
        }

        let w = XmlWriter.initPretty(alloc, 2);
        defer w.deinit();

        w.writeDeclaration() catch return SitemapError.OutOfMemory;
        w.openTag("sitemapindex") catch return SitemapError.OutOfMemory;
        w.attr("xmlns", "http://www.sitemaps.org/schemas/sitemap/0.9") catch return SitemapError.OutOfMemory;

        for (item of items) {
            w.openTag("sitemap") catch return SitemapError.OutOfMemory;

            w.openTag("loc") catch return SitemapError.OutOfMemory;
            w.text(item.loc) catch return SitemapError.OutOfMemory;
            w.closeTag() catch return SitemapError.OutOfMemory;

            if (item.lastmod.len > 0) {
                w.openTag("lastmod") catch return SitemapError.OutOfMemory;
                w.text(item.lastmod) catch return SitemapError.OutOfMemory;
                w.closeTag() catch return SitemapError.OutOfMemory;
            }

            w.closeTag() catch return SitemapError.OutOfMemory;
        }

        w.closeTag() catch return SitemapError.OutOfMemory;
        return w.toOwnedSlice() catch return SitemapError.OutOfMemory;
    }
}

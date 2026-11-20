// Source: cp/lib/dns/resolv_conf.cp (POSIX /etc/resolv.conf Parser)
// lib/dns/resolv_conf.cp
// POSIX /etc/resolv.conf Parser, Host Loader & Search-Domain/ndots Expander

const std = @import("std");
import { readFile } from "cap:sys";

pub const MAX_NAMESERVERS: usize = 16;
pub const MAX_SEARCH_DOMAINS: usize = 16;
pub const DEFAULT_NDOTS: u8 = 1;
pub const DEFAULT_TIMEOUT: u16 = 5;
pub const DEFAULT_ATTEMPTS: u8 = 2;

/// Zero-allocation parser for POSIX `/etc/resolv.conf` files, plus optional
/// host file loading (`loadSystem` / `loadFromFile`) and `ndots` search expansion.
pub struct ResolvConf {
    pub nameservers: [16][]const u8 = [""; 16];
    pub nameserver_count: usize = 0;
    pub search_domains: [16][]const u8 = [""; 16];
    pub search_count: usize = 0;
    pub domain: []const u8 = "";
    pub ndots: u8 = DEFAULT_NDOTS;
    pub timeout: u16 = DEFAULT_TIMEOUT;
    pub attempts: u8 = DEFAULT_ATTEMPTS;
    pub rotate: bool = false;
    pub edns0: bool = false;
    pub trust_ad: bool = false;
    pub use_vc: bool = false;
    priv owned_buf: ([]u8)? = null;

    pub static fn init(): ResolvConf {
        return ResolvConf{};
    }

    pub fn deinit(*_, alloc: std.mem.Allocator) {
        if (buf := _.owned_buf) {
            alloc.free(buf);
            _.owned_buf = null;
        }
    }

    pub fn getNameservers(*_): []const []const u8 {
        return _.nameservers[0.._.nameserver_count];
    }

    pub fn getSearchDomains(*_): []const []const u8 {
        return _.search_domains[0.._.search_count];
    }

    static fn isWhitespace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\r';
    }

    static fn stripComment(line: []const u8): []const u8 {
        let i: usize = 0;
        while (i < line.len) {
            if (line[i] == '#' || line[i] == ';') {
                return line[0..i];
            }
            i += 1;
        }
        return line;
    }

    static fn nextToken(line: []const u8, cursor: *usize): ([]const u8)? {
        let pos = cursor.*;
        while (pos < line.len && ResolvConf.isWhitespace(line[pos])) {
            pos += 1;
        }
        if (pos >= line.len) {
            cursor.* = pos;
            return null;
        }
        const start = pos;
        while (pos < line.len && !ResolvConf.isWhitespace(line[pos])) {
            pos += 1;
        }
        cursor.* = pos;
        return line[start..pos];
    }

    static fn isPlausibleNameserver(tok: []const u8): bool {
        if (tok.len == 0) return false;
        let has_dot = false;
        let has_colon = false;
        for (c of tok) {
            if (c == '.') has_dot = true;
            if (c == ':') has_colon = true;
        }
        return has_dot || has_colon;
    }

    static fn parseUnsignedOption(val_str: []const u8): u16? {
        if (val_str.len == 0) return null;
        return std.fmt.parseInt(u16, val_str, 10) catch null;
    }

    /// Parses `/etc/resolv.conf` text without heap allocation.
    /// Slices in `nameservers`, `search_domains`, and `domain` borrow from `text`.
    pub static fn parse(text: []const u8): ResolvConf {
        let cfg = ResolvConf.init();
        let line_iter = std.mem.splitScalar(u8, text, '\n');

        while (raw_line := line_iter.next()) {
            const clean_line = ResolvConf.stripComment(raw_line);
            let cur: usize = 0;
            const keyword = ResolvConf.nextToken(clean_line, *cur) ?? continue;

            if (std.mem.eql(u8, keyword, "nameserver")) {
                if (ns := ResolvConf.nextToken(clean_line, *cur)) {
                    if (ResolvConf.isPlausibleNameserver(ns) && cfg.nameserver_count < MAX_NAMESERVERS) {
                        cfg.nameservers[cfg.nameserver_count] = ns;
                        cfg.nameserver_count += 1;
                    }
                }
            } else if (std.mem.eql(u8, keyword, "search")) {
                cfg.search_count = 0;
                while (dom := ResolvConf.nextToken(clean_line, *cur)) {
                    if (cfg.search_count < MAX_SEARCH_DOMAINS) {
                        cfg.search_domains[cfg.search_count] = dom;
                        cfg.search_count += 1;
                    }
                }
            } else if (std.mem.eql(u8, keyword, "domain")) {
                if (dom := ResolvConf.nextToken(clean_line, *cur)) {
                    cfg.domain = dom;
                    cfg.search_domains[0] = dom;
                    cfg.search_count = 1;
                }
            } else if (std.mem.eql(u8, keyword, "options")) {
                while (opt := ResolvConf.nextToken(clean_line, *cur)) {
                    if (std.mem.startsWith(u8, opt, "ndots:")) {
                        if (v := ResolvConf.parseUnsignedOption(opt[6..])) {
                            cfg.ndots = if (v > 15) 15 else (v as u8);
                        }
                    } else if (std.mem.startsWith(u8, opt, "timeout:")) {
                        if (v := ResolvConf.parseUnsignedOption(opt[8..])) {
                            cfg.timeout = if (v > 30) 30 else v;
                        }
                    } else if (std.mem.startsWith(u8, opt, "attempts:")) {
                        if (v := ResolvConf.parseUnsignedOption(opt[9..])) {
                            cfg.attempts = if (v > 5) 5 else (v as u8);
                        }
                    } else if (std.mem.eql(u8, opt, "rotate")) {
                        cfg.rotate = true;
                    } else if (std.mem.eql(u8, opt, "edns0")) {
                        cfg.edns0 = true;
                    } else if (std.mem.eql(u8, opt, "trust-ad")) {
                        cfg.trust_ad = true;
                    } else if (std.mem.eql(u8, opt, "use-vc")) {
                        cfg.use_vc = true;
                    }
                }
            }
        }
        return cfg;
    }

    /// Loads and parses a resolv.conf file from disk (`deinit(alloc)` must be called).
    pub static fn loadFromFile(alloc: std.mem.Allocator, path: []const u8): ResolvConf! {
        const content = readFile(alloc, path)?;
        errdefer alloc.free(content);

        let cfg = ResolvConf.parse(content);
        cfg.owned_buf = content;
        return cfg;
    }

    /// Loads and parses the host `/etc/resolv.conf` (`deinit(alloc)` must be called).
    pub static fn loadSystem(alloc: std.mem.Allocator): ResolvConf! {
        return ResolvConf.loadFromFile(alloc, "/etc/resolv.conf");
    }

    /// Loads `/etc/resolv.conf` if present and readable, otherwise returns `null`.
    pub static fn loadSystemOrNull(alloc: std.mem.Allocator): ResolvConf? {
        return ResolvConf.loadSystem(alloc) catch return null;
    }

    static fn countDots(host: []const u8): usize {
        let dots: usize = 0;
        for (c of host) {
            if (c == '.') dots += 1;
        }
        return dots;
    }

    /// Expands candidate lookup names for `host` according to POSIX `search` and `options ndots:N`.
    /// Free the returned slice with `ResolvConf.freeExpandedDomains(alloc, list)`.
    pub fn expandSearchDomains(alloc: std.mem.Allocator, host: []const u8): ([][]u8)! {
        let list: std.ArrayList([]u8) = .empty;
        errdefer {
            for (item of list.items) alloc.free(item);
            list.deinit(alloc);
        }

        if (host.len == 0) {
            return list.toOwnedSlice(alloc);
        }

        // Trailing dot denotes an absolute FQDN: do not append search domains.
        if (host[host.len - 1] == '.') {
            const fqdn = alloc.dupe(u8, host)?;
            errdefer alloc.free(fqdn);
            list.append(alloc, fqdn)?;
            return list.toOwnedSlice(alloc);
        }

        const dots = ResolvConf.countDots(host);
        const try_as_is_first = dots >= (_.ndots as usize);

        if (try_as_is_first || _.search_count == 0) {
            const first = alloc.dupe(u8, host)?;
            errdefer alloc.free(first);
            list.append(alloc, first)?;
        }

        let i: usize = 0;
        while (i < _.search_count) {
            const dom = _.search_domains[i];
            if (dom.len > 0) {
                const candidate = std.fmt.allocPrint(alloc, "{s}.{s}", .{ host, dom })?;
                errdefer alloc.free(candidate);
                list.append(alloc, candidate)?;
            }
            i += 1;
        }

        if (!try_as_is_first && _.search_count > 0) {
            const last = alloc.dupe(u8, host)?;
            errdefer alloc.free(last);
            list.append(alloc, last)?;
        }

        return list.toOwnedSlice(alloc);
    }

    pub static fn freeExpandedDomains(alloc: std.mem.Allocator, list: [][]u8) {
        for (item of list) {
            alloc.free(item);
        }
        if (list.len > 0) {
            alloc.free(list);
        }
    }
}

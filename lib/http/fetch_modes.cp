// Source: Bun path-not-recorded
// lib/http/fetch_modes.cp
// CP Web Fetch Request Enums & Modes (cap:http)
// Ported from Bun.js http_types (FetchCacheMode, FetchRedirect, FetchRequestMode)

const std = @import("std");

pub enum FetchCacheMode: u8 {
    Default = 0,
    NoStore = 1,
    Reload = 2,
    NoCache = 3,
    ForceCache = 4,
    OnlyIfCached = 5,

    pub static fn fromString(str: []const u8): FetchCacheMode? {
        if (std.mem.eql(u8, str, "default")) { return FetchCacheMode.Default; }
        if (std.mem.eql(u8, str, "no-store")) { return FetchCacheMode.NoStore; }
        if (std.mem.eql(u8, str, "reload")) { return FetchCacheMode.Reload; }
        if (std.mem.eql(u8, str, "no-cache")) { return FetchCacheMode.NoCache; }
        if (std.mem.eql(u8, str, "force-cache")) { return FetchCacheMode.ForceCache; }
        if (std.mem.eql(u8, str, "only-if-cached")) { return FetchCacheMode.OnlyIfCached; }
        return null;
    }
}

pub enum FetchRedirect: u8 {
    Follow = 0,
    Manual = 1,
    Error = 2,

    pub static fn fromString(str: []const u8): FetchRedirect? {
        if (std.mem.eql(u8, str, "follow")) { return FetchRedirect.Follow; }
        if (std.mem.eql(u8, str, "manual")) { return FetchRedirect.Manual; }
        if (std.mem.eql(u8, str, "error")) { return FetchRedirect.Error; }
        return null;
    }
}

pub enum FetchRequestMode: u8 {
    SameOrigin = 0,
    NoCors = 1,
    Cors = 2,
    Navigate = 3,

    pub static fn fromString(str: []const u8): FetchRequestMode? {
        if (std.mem.eql(u8, str, "same-origin")) { return FetchRequestMode.SameOrigin; }
        if (std.mem.eql(u8, str, "no-cors")) { return FetchRequestMode.NoCors; }
        if (std.mem.eql(u8, str, "cors")) { return FetchRequestMode.Cors; }
        if (std.mem.eql(u8, str, "navigate")) { return FetchRequestMode.Navigate; }
        return null;
    }
}

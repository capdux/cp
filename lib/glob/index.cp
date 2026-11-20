// Source: /data/dev/bun/src/glob/glob.cp
// lib/glob/index.cp

import { Matcher, GlobMatcher, GlobMatchOptions } from "./matcher.cp";
import { GlobWalker, GlobWalkerOptions } from "./walker.cp";

export { Matcher, GlobMatcher, GlobMatchOptions };
export { GlobWalker, GlobWalkerOptions };

pub const Glob = struct {
    pub const match = Matcher.match;
    pub const matchWithOptions = Matcher.matchWithOptions;
    pub const hasMagic = Matcher.hasMagic;
    pub const normalizePath = Matcher.normalizePath;
    pub const Walker = GlobWalker;
    pub const scan = GlobWalker.scan;
    pub const scanSync = GlobWalker.scanSync;
};

pub const glob = Glob;

// Source: bun/src/glob/
// lib/glob/index.cp

import { Matcher, GlobMatcher } from "./matcher.cp";
import { GlobWalker, GlobWalkerOptions } from "./walker.cp";

export { Matcher, GlobMatcher };
export { GlobWalker, GlobWalkerOptions };

pub const Glob = struct {
    pub const match = Matcher.match;
    pub const hasMagic = Matcher.hasMagic;
    pub const Walker = GlobWalker;
    pub const scan = GlobWalker.scan;
};

pub const glob = Glob;

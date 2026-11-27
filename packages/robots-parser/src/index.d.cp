// Type definitions for CP

export fn toLowerAscii(c: u8): u8;
export fn isSpaceOrTab(c: u8): bool;
export fn trimSpaces(s: &[]u8): &[]u8;
export fn eqIgnoreCase(a: &[]u8, b: &[]u8): bool;
export fn startsWithIgnoreCase(haystack: &[]u8, prefix: &[]u8): bool;
export fn stripBom(src: &[]u8): &[]u8;
export fn cleanLine(raw: &[]u8): &[]u8;
export fn findColon(line: &[]u8): usize;
export fn patMatches(pattern: &[]u8, path: &[]u8): bool;
export fn parseDelaySeconds(val: &[]u8): i64;
export fn uaMatchScore(ruleUa: &[]u8, targetUa: &[]u8): i64;
export fn isAllowed(robots_txt: &[]u8, user_agent: &[]u8, path: &[]u8): bool;
export fn crawlDelay(robots_txt: &[]u8, user_agent: &[]u8): i64;
export fn firstSitemap(robots_txt: &[]u8): &[]u8;

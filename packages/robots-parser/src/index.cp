// packages/@cap/robots/src/index.cp
// Zero-allocation RFC 9309 robots.txt parser and URL compliance engine.
// MIT clean-room pure CP implementation.

import { eqIgnoreCase, findByte } from "cap:ascii";

fn isSpaceOrTab(c: u8): bool {
    return c == (' ' as u8) || c == ('\t' as u8);
}

fn trimSpaces(s: []const u8): []const u8 {
    let start: usize = 0;
    let end: usize = s.len;
    while (start < end && isSpaceOrTab(s[start])) {
        start += 1;
    }
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn startsWithIgnoreCase(haystack: []const u8, prefix: []const u8): bool {
    if (prefix.len == 0 || haystack.len < prefix.len) {
        return false;
    }
    return eqIgnoreCase(haystack[0..prefix.len], prefix);
}

/// Strip UTF-8 BOM if present at the start of robots.txt.
fn stripBom(src: []const u8): []const u8 {
    if (src.len >= 3 && src[0] == (0xEF as u8) && src[1] == (0xBB as u8) && src[2] == (0xBF as u8)) {
        return src[3..src.len];
    }
    return src;
}

/// Strip trailing CR and '#' comment, then trim leading/trailing spaces and tabs.
fn cleanLine(raw: []const u8): []const u8 {
    let end = raw.len;
    if (end > 0 && raw[end - 1] == ('\r' as u8)) {
        end -= 1;
    }
    if (hash := findByte(raw[0..end], '#' as u8, 0)) {
        end = hash;
    }
    return trimSpaces(raw[0..end]);
}

/// Find the index of the first ':' in a cleaned line, or line.len if absent.
fn findColon(line: []const u8): usize {
    if (idx := findByte(line, ':' as u8, 0)) {
        return idx;
    }
    return line.len;
}

/// RFC 9309 wildcard '*' and end-anchor '$' pattern matching against a URL path.
fn patMatches(pattern: []const u8, path: []const u8): bool {
    if (pattern.len == 0) {
        return false;
    }
    let anchored = false;
    let patLen = pattern.len;
    if (pattern[patLen - 1] == ('$' as u8)) {
        anchored = true;
        patLen -= 1;
    }

    let pi: usize = 0;
    let si: usize = 0;
    let hasStar = false;
    let starPi: usize = 0;
    let starSi: usize = 0;

    while (true) {
        if (pi < patLen && pattern[pi] == ('*' as u8)) {
            hasStar = true;
            starPi = pi;
            starSi = si;
            pi += 1;
        } else if (pi < patLen && si < path.len && pattern[pi] == path[si]) {
            pi += 1;
            si += 1;
        } else if (pi == patLen) {
            if (!anchored) {
                return true;
            }
            if (si == path.len) {
                return true;
            }
            if (hasStar && starSi < path.len) {
                starSi += 1;
                si = starSi;
                pi = starPi + 1;
            } else {
                return false;
            }
        } else {
            if (hasStar && starSi < path.len) {
                starSi += 1;
                si = starSi;
                pi = starPi + 1;
            } else {
                return false;
            }
        }
    }
    return false;
}

/// Parse a non-negative integer seconds value for Crawl-delay (truncates optional fractional part).
/// Returns -1 if invalid.
fn parseDelaySeconds(val: []const u8): i64 {
    if (val.len == 0) {
        return -1 as i64;
    }
    let i: usize = 0;
    let acc: i64 = 0;
    let hasDigit = false;
    while (i < val.len) {
        const c = val[i];
        if (c >= ('0' as u8) && c <= ('9' as u8)) {
            hasDigit = true;
            if (acc <= (1000000000 as i64)) {
                acc = acc * (10 as i64) + ((c - ('0' as u8)) as i64);
            }
            i += 1;
        } else if (c == ('.' as u8)) {
            // Validate remaining fractional digits.
            i += 1;
            while (i < val.len) {
                const fc = val[i];
                if (fc < ('0' as u8) || fc > ('9' as u8)) {
                    return -1 as i64;
                }
                i += 1;
            }
            break;
        } else {
            return -1 as i64;
        }
    }
    if (!hasDigit) {
        return -1 as i64;
    }
    return acc;
}

/// Check if a User-agent directive value matches the target user_agent.
/// Returns: -1 (no match), 0 (wildcard '*'), or >0 (specific match length).
fn uaMatchScore(ruleUa: []const u8, targetUa: []const u8): i64 {
    if (ruleUa.len == 0) {
        return -1 as i64;
    }
    if (ruleUa.len == 1 && ruleUa[0] == ('*' as u8)) {
        return 0 as i64;
    }
    if (startsWithIgnoreCase(targetUa, ruleUa)) {
        return ruleUa.len as i64;
    }
    return -1 as i64;
}

/// Evaluate whether `path` is allowed for `user_agent` under RFC 9309 `robots_txt`.
/// - Matches the most specific `User-agent` group (case-insensitive), falling back to `*`.
/// - Within matching groups, longest matching rule wins; ties favor `Allow` over `Disallow`.
/// - Empty `Disallow:` allows all paths. Supports `*` wildcard and `$` end anchor.
pub fn isAllowed(robots_txt: []const u8, user_agent: []const u8, path: []const u8): bool {
    const src = stripBom(robots_txt);
    let pos: usize = 0;

    let seenRule = false;
    let curGroupSpecScore: i64 = -1 as i64;
    let curGroupWildcard = false;

    let bestSpecScore: i64 = 0 as i64;
    let specAllowLen: usize = 0;
    let specDisallowLen: usize = 0;

    let wildAllowLen: usize = 0;
    let wildDisallowLen: usize = 0;

    while (pos < src.len) {
        let lineEnd = pos;
        while (lineEnd < src.len && src[lineEnd] != ('\n' as u8)) {
            lineEnd += 1;
        }
        const line = cleanLine(src[pos..lineEnd]);
        if (lineEnd < src.len) {
            pos = lineEnd + 1;
        } else {
            pos = src.len;
        }

        if (line.len == 0) {
            continue;
        }
        const colon = findColon(line);
        if (colon >= line.len) {
            continue;
        }

        const field = trimSpaces(line[0..colon]);
        const value = trimSpaces(line[(colon + 1)..line.len]);

        if (eqIgnoreCase(field, "user-agent")) {
            if (seenRule) {
                seenRule = false;
                curGroupSpecScore = -1 as i64;
                curGroupWildcard = false;
            }
            const score = uaMatchScore(value, user_agent);
            if (score == (0 as i64)) {
                curGroupWildcard = true;
            } else if (score > (0 as i64)) {
                if (score > curGroupSpecScore) {
                    curGroupSpecScore = score;
                }
                if (score > bestSpecScore) {
                    bestSpecScore = score;
                    specAllowLen = 0;
                    specDisallowLen = 0;
                }
            }
        } else if (eqIgnoreCase(field, "allow")) {
            seenRule = true;
            if (value.len > 0 && patMatches(value, path)) {
                if (curGroupSpecScore > (0 as i64) && curGroupSpecScore == bestSpecScore) {
                    if (value.len > specAllowLen) {
                        specAllowLen = value.len;
                    }
                }
                if (curGroupWildcard) {
                    if (value.len > wildAllowLen) {
                        wildAllowLen = value.len;
                    }
                }
            }
        } else if (eqIgnoreCase(field, "disallow")) {
            seenRule = true;
            if (value.len > 0 && patMatches(value, path)) {
                if (curGroupSpecScore > (0 as i64) && curGroupSpecScore == bestSpecScore) {
                    if (value.len > specDisallowLen) {
                        specDisallowLen = value.len;
                    }
                }
                if (curGroupWildcard) {
                    if (value.len > wildDisallowLen) {
                        wildDisallowLen = value.len;
                    }
                }
            }
        } else if (eqIgnoreCase(field, "crawl-delay")) {
            seenRule = true;
        }
    }

    if (bestSpecScore > (0 as i64)) {
        if (specAllowLen == 0 && specDisallowLen == 0) {
            return true;
        }
        return specAllowLen >= specDisallowLen;
    }

    if (wildAllowLen == 0 && wildDisallowLen == 0) {
        return true;
    }
    return wildAllowLen >= wildDisallowLen;
}

/// Extract the `Crawl-delay` in whole seconds for the matching `User-agent` group.
/// Returns -1 if not specified.
pub fn crawlDelay(robots_txt: []const u8, user_agent: []const u8): i64 {
    const src = stripBom(robots_txt);
    let pos: usize = 0;

    let seenRule = false;
    let curGroupSpecScore: i64 = -1 as i64;
    let curGroupWildcard = false;

    let bestSpecScore: i64 = 0 as i64;
    let specDelay: i64 = -1 as i64;
    let wildDelay: i64 = -1 as i64;

    while (pos < src.len) {
        let lineEnd = pos;
        while (lineEnd < src.len && src[lineEnd] != ('\n' as u8)) {
            lineEnd += 1;
        }
        const line = cleanLine(src[pos..lineEnd]);
        if (lineEnd < src.len) {
            pos = lineEnd + 1;
        } else {
            pos = src.len;
        }

        if (line.len == 0) {
            continue;
        }
        const colon = findColon(line);
        if (colon >= line.len) {
            continue;
        }

        const field = trimSpaces(line[0..colon]);
        const value = trimSpaces(line[(colon + 1)..line.len]);

        if (eqIgnoreCase(field, "user-agent")) {
            if (seenRule) {
                seenRule = false;
                curGroupSpecScore = -1 as i64;
                curGroupWildcard = false;
            }
            const score = uaMatchScore(value, user_agent);
            if (score == (0 as i64)) {
                curGroupWildcard = true;
            } else if (score > (0 as i64)) {
                if (score > curGroupSpecScore) {
                    curGroupSpecScore = score;
                }
                if (score > bestSpecScore) {
                    bestSpecScore = score;
                    specDelay = -1 as i64;
                }
            }
        } else if (eqIgnoreCase(field, "crawl-delay")) {
            seenRule = true;
            const parsed = parseDelaySeconds(value);
            if (parsed >= (0 as i64)) {
                if (curGroupSpecScore > (0 as i64) && curGroupSpecScore == bestSpecScore) {
                    if (specDelay < (0 as i64)) {
                        specDelay = parsed;
                    }
                }
                if (curGroupWildcard) {
                    if (wildDelay < (0 as i64)) {
                        wildDelay = parsed;
                    }
                }
            }
        } else if (eqIgnoreCase(field, "allow") || eqIgnoreCase(field, "disallow")) {
            seenRule = true;
        }
    }

    if (bestSpecScore > (0 as i64)) {
        return specDelay;
    }
    return wildDelay;
}

/// Zero-copy extraction of the first `Sitemap:` URL directive in `robots_txt`.
/// Returns empty slice `""` if not found.
pub fn firstSitemap(robots_txt: []const u8): []const u8 {
    const src = stripBom(robots_txt);
    let pos: usize = 0;

    while (pos < src.len) {
        let lineEnd = pos;
        while (lineEnd < src.len && src[lineEnd] != ('\n' as u8)) {
            lineEnd += 1;
        }
        const line = cleanLine(src[pos..lineEnd]);
        if (lineEnd < src.len) {
            pos = lineEnd + 1;
        } else {
            pos = src.len;
        }

        if (line.len == 0) {
            continue;
        }
        const colon = findColon(line);
        if (colon >= line.len) {
            continue;
        }

        const field = trimSpaces(line[0..colon]);
        const value = trimSpaces(line[(colon + 1)..line.len]);
        if (eqIgnoreCase(field, "sitemap") && value.len > 0) {
            return value;
        }
    }

    return "";
}

export {
    isAllowed,
    crawlDelay,
    firstSitemap,
};

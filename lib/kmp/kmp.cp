// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linear-Time KMP, Z-Algorithm & Sentinel-Free Manacher Palindrome Algorithms)

/// Span describing a palindromic substring `s[start .. start + len]`.
pub struct PalindromeSpan {
    start: usize;
    len: usize;
}

/// Computes the KMP prefix function (failure table) `pi[0..pattern.len]`, where `pi[i]` is
/// the length of the longest proper prefix of `pattern[0..i + 1]` that is also a suffix.
///
/// Returns `false` if `pi_out.len < pattern.len`.
pub fn computePrefixFunction(pattern: []const u8, pi_out: []usize): bool {
    const m = pattern.len;
    if (pi_out.len < m) {
        return false;
    }
    if (m == 0) {
        return true;
    }
    pi_out[0] = 0;
    let i: usize = 1;
    while (i < m) {
        let j = pi_out[i - 1];
        while (j > 0 && pattern[i] != pattern[j]) {
            j = pi_out[j - 1];
        }
        if (pattern[i] == pattern[j]) {
            j += 1;
        }
        pi_out[i] = j;
        i += 1;
    }
    return true;
}

/// Finds the first occurrence of `pattern` in `text` in O(n) time using precomputed prefix table `pi`.
///
/// Returns `0` for an empty pattern, or `null` if not found, `pattern.len > text.len`, or `pi.len < pattern.len`.
pub fn kmpFindFirst(text: []const u8, pattern: []const u8, pi: []const usize): usize? {
    const m = pattern.len;
    if (m == 0) {
        return 0;
    }
    if (pi.len < m || m > text.len) {
        return null;
    }
    let j: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        while (j > 0 && text[i] != pattern[j]) {
            j = pi[j - 1];
        }
        if (text[i] == pattern[j]) {
            j += 1;
        }
        if (j == m) {
            return i + 1 - m;
        }
        i += 1;
    }
    return null;
}

/// Finds all (including overlapping) occurrences of `pattern` in `text` in O(n) time,
/// writing starting indices into `matches_out` up to `matches_out.len`.
///
/// Returns the number of match indices written to `matches_out`.
pub fn kmpFindAll(
    text: []const u8,
    pattern: []const u8,
    pi: []const usize,
    matches_out: []usize,
): usize {
    const m = pattern.len;
    if (m == 0 || pi.len < m || m > text.len) {
        return 0;
    }
    let written: usize = 0;
    let j: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        while (j > 0 && text[i] != pattern[j]) {
            j = pi[j - 1];
        }
        if (text[i] == pattern[j]) {
            j += 1;
        }
        if (j == m) {
            if (written < matches_out.len) {
                matches_out[written] = i + 1 - m;
                written += 1;
            }
            j = pi[j - 1];
        }
        i += 1;
    }
    return written;
}

/// Counts all (including overlapping) occurrences of `pattern` in `text` in O(n) time
/// with zero output buffer required.
pub fn kmpCount(text: []const u8, pattern: []const u8, pi: []const usize): usize {
    const m = pattern.len;
    if (m == 0 || pi.len < m || m > text.len) {
        return 0;
    }
    let count: usize = 0;
    let j: usize = 0;
    let i: usize = 0;
    while (i < text.len) {
        while (j > 0 && text[i] != pattern[j]) {
            j = pi[j - 1];
        }
        if (text[i] == pattern[j]) {
            j += 1;
        }
        if (j == m) {
            count += 1;
            j = pi[j - 1];
        }
        i += 1;
    }
    return count;
}

/// Computes the shortest period length `p = s.len - pi[s.len - 1]` of `s` using its prefix table `pi`.
///
/// Returns `0` for an empty string, or `s.len` if `pi` is undersized or malformed.
pub fn shortestPeriod(s: []const u8, pi: []const usize): usize {
    const n = s.len;
    if (n == 0) {
        return 0;
    }
    if (pi.len < n) {
        return n;
    }
    const border = pi[n - 1];
    if (border >= n) {
        return n;
    }
    return n - border;
}

/// Computes the Z-array `z_out[0..s.len]` in O(n) time, where `z_out[i] = lcp(s, s[i..])`.
///
/// Specifically, `z_out[0] == s.len` for non-empty `s`.
/// Returns `false` if `z_out.len < s.len`.
pub fn computeZArray(s: []const u8, z_out: []usize): bool {
    const n = s.len;
    if (z_out.len < n) {
        return false;
    }
    if (n == 0) {
        return true;
    }
    z_out[0] = n;
    let l: usize = 0;
    let r: usize = 0;
    let i: usize = 1;
    while (i < n) {
        let zi: usize = 0;
        if (i <= r) {
            const rem = r - i + 1;
            const mirror = z_out[i - l];
            zi = if (rem < mirror) rem else mirror;
        }
        while (i + zi < n && s[zi] == s[i + zi]) {
            zi += 1;
        }
        z_out[i] = zi;
        if (zi > 0 && i + zi - 1 > r) {
            l = i;
            r = i + zi - 1;
        }
        i += 1;
    }
    return true;
}

fn computeManacherRadii(s: []const u8, d1: []usize, d2: []usize): bool {
    const n = s.len;
    if (d1.len < n || d2.len < n) {
        return false;
    }
    if (n == 0) {
        return true;
    }

    // Odd-length palindromes centered at i: s[i - d1[i] + 1 .. i + d1[i]]
    let has_box = false;
    let l: usize = 0;
    let r: usize = 0;
    let i: usize = 0;
    while (i < n) {
        let k: usize = 1;
        if (has_box && i <= r) {
            const mirror = d1[l + r - i];
            const bound = r - i + 1;
            k = if (mirror < bound) mirror else bound;
        }
        while (i >= k && i + k < n && s[i - k] == s[i + k]) {
            k += 1;
        }
        d1[i] = k;
        if (!has_box || i + k - 1 > r) {
            has_box = true;
            l = i + 1 - k;
            r = i + k - 1;
        }
        i += 1;
    }

    // Even-length palindromes centered between i - 1 and i: s[i - d2[i] .. i + d2[i]]
    has_box = false;
    l = 0;
    r = 0;
    i = 0;
    while (i < n) {
        let k: usize = 0;
        if (has_box && i <= r) {
            const mirror = d2[l + r - i + 1];
            const bound = r - i + 1;
            k = if (mirror < bound) mirror else bound;
        }
        while (i > k && i + k < n && s[i - k - 1] == s[i + k]) {
            k += 1;
        }
        d2[i] = k;
        if (k > 0 && (!has_box || i + k - 1 > r)) {
            has_box = true;
            l = i - k;
            r = i + k - 1;
        }
        i += 1;
    }

    return true;
}

/// Finds the longest palindromic substring of arbitrary byte slice `s` in O(n) time
/// using sentinel-free Manacher's algorithm (safe for binary data containing `\x00`..`\xff`).
///
/// Requires `d1_scratch.len >= s.len` and `d2_scratch.len >= s.len`.
/// Returns `null` if either scratch buffer is undersized.
pub fn manacherLongestPalindrome(
    s: []const u8,
    d1_scratch: []usize,
    d2_scratch: []usize,
): PalindromeSpan? {
    if (!computeManacherRadii(s, d1_scratch, d2_scratch)) {
        return null;
    }
    const n = s.len;
    if (n == 0) {
        return PalindromeSpan { start: 0, len: 0 };
    }

    let best_start: usize = 0;
    let best_len: usize = 0;
    let i: usize = 0;
    while (i < n) {
        const odd_len = (2 as usize) * d1_scratch[i] - 1;
        if (odd_len > best_len) {
            best_len = odd_len;
            best_start = i + 1 - d1_scratch[i];
        }
        const even_len = (2 as usize) * d2_scratch[i];
        if (even_len > best_len) {
            best_len = even_len;
            best_start = i - d2_scratch[i];
        }
        i += 1;
    }

    return PalindromeSpan {
        start: best_start,
        len: best_len,
    };
}

/// Counts the total number of non-empty palindromic substrings in `s` in O(n) time.
///
/// Requires `d1_scratch.len >= s.len` and `d2_scratch.len >= s.len`.
/// Returns `null` if either scratch buffer is undersized.
pub fn countPalindromicSubstrings(
    s: []const u8,
    d1_scratch: []usize,
    d2_scratch: []usize,
): usize? {
    if (!computeManacherRadii(s, d1_scratch, d2_scratch)) {
        return null;
    }
    let total: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        total += d1_scratch[i] + d2_scratch[i];
        i += 1;
    }
    return total;
}

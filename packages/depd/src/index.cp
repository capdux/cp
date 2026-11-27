// depd — Zero-heap deprecation notice formatter & deduplicated warning tracker
// Matching npm `depd` (150M+/wk). Clean-room MIT in pure CP.

import { eqIgnoreCase, trim } from "cap:ascii";
import { fnv64OffsetBasis, mulFnv64Prime } from "../../@sindresorhus/fnv1a/src/index.cp";

pub const MAX_SEEN_HASHES: usize = 64;

pub struct DepdOptions {
    pub no_deprecation: bool,
    pub trace_deprecation: bool,

    pub static fn defaults(): DepdOptions {
        return DepdOptions {
            no_deprecation: false,
            trace_deprecation: false,
        };
    }
}

pub fn depdHashKey(ns: string, msg: string): u64 {
    let h: u64 = fnv64OffsetBasis();
    let i: usize = 0;
    while (i < ns.len) {
        h = mulFnv64Prime(h ^ (ns[i] as u64));
        i = i + 1;
    }
    h = mulFnv64Prime(h ^ 58);
    let j: usize = 0;
    while (j < msg.len) {
        h = mulFnv64Prime(h ^ (msg[j] as u64));
        j = j + 1;
    }
    return h;
}

pub fn copySlice(dst: []u8, pos: usize, src: string): usize {
    if (pos >= dst.len || src.len == 0) {
        return pos;
    }
    let avail = dst.len - pos;
    let n = if (src.len < avail) { src.len } else { avail };
    @memcpy(dst[pos..pos + n], src[0..n]);
    return pos + n;
}

/// Checks whether `namespace` matches a comma-separated environment list
/// (such as `NO_DEPRECATION` or `TRACE_DEPRECATION`), supporting `*` wildcard.
fn matchNamespaceList(namespace: string, env_list: string): bool {
    if (env_list.len == 0 || namespace.len == 0) {
        return false;
    }
    let start: usize = 0;
    let i: usize = 0;
    while (i <= env_list.len) {
        if (i == env_list.len || env_list[i] == ',') {
            const token = trim(env_list[start..i]);
            if (token.len > 0) {
                if ((token.len == 1 && token[0] == '*') || eqIgnoreCase(token, namespace)) {
                    return true;
                }
            }
            start = i + 1;
        }
        i = i + 1;
    }
    return false;
}

/// Returns true if `namespace` is silenced by a `NO_DEPRECATION` comma-separated list.
pub fn isNamespaceSilenced(namespace: string, no_deprecation_env: string): bool {
    return matchNamespaceList(namespace, no_deprecation_env);
}

/// Returns true if `namespace` has stack traces enabled by a `TRACE_DEPRECATION` list.
pub fn isNamespaceTraced(namespace: string, trace_deprecation_env: string): bool {
    return matchNamespaceList(namespace, trace_deprecation_env);
}

pub struct Deprecator {
    pub namespace: string,
    pub silenced: bool,
    pub no_deprecation: bool,
    pub trace_deprecation: bool,
    pub seen_hashes: [64]u64,
    pub seen_count: usize,
    pub emitted_count: u64,

    pub static fn new(namespace: string): Deprecator {
        return Deprecator {
            namespace: namespace,
            silenced: false,
            no_deprecation: false,
            trace_deprecation: false,
            seen_hashes: undefined,
            seen_count: 0,
            emitted_count: 0,
        };
    }

    pub static fn withOptions(namespace: string, opts: DepdOptions): Deprecator {
        return Deprecator {
            namespace: namespace,
            silenced: opts.no_deprecation,
            no_deprecation: opts.no_deprecation,
            trace_deprecation: opts.trace_deprecation,
            seen_hashes: undefined,
            seen_count: 0,
            emitted_count: 0,
        };
    }

    pub fn setSilenced(*_, flag: bool) {
        _.silenced = flag;
        _.no_deprecation = flag;
    }

    pub fn setNoDeprecation(*_, flag: bool) {
        _.no_deprecation = flag;
        _.silenced = flag;
    }

    pub fn setTraceDeprecation(*_, flag: bool) {
        _.trace_deprecation = flag;
    }

    pub fn isSilenced(): bool {
        return _.silenced || _.no_deprecation;
    }

    pub fn isTraceEnabled(): bool {
        return _.trace_deprecation;
    }

    pub fn seenCount(): usize {
        return _.seen_count;
    }

    pub fn hasWarned(msg: string): bool {
        let h = depdHashKey(_.namespace, msg);
        let i: usize = 0;
        while (i < _.seen_count) {
            if (_.seen_hashes[i] == h) { return true; }
            i = i + 1;
        }
        return false;
    }

    // Formats "<namespace> deprecated <msg> at <site>" once per unique message.
    // Returns 0 if silenced or already warned for this message.
    pub fn warnOnce(*_, msg: string, site: string, out: []u8): usize {
        if (_.silenced || _.no_deprecation) { return 0; }
        let h = depdHashKey(_.namespace, msg);
        let i: usize = 0;
        while (i < _.seen_count) {
            if (_.seen_hashes[i] == h) { return 0; }
            i = i + 1;
        }
        if (_.seen_count < MAX_SEEN_HASHES) {
            _.seen_hashes[_.seen_count] = h;
            _.seen_count = _.seen_count + 1;
        }
        _.emitted_count = _.emitted_count + 1;
        return formatDeprecation(_.namespace, msg, site, out);
    }

    // Formats deprecation message once, appending stack trace when `trace_deprecation` is enabled.
    pub fn warnWithTrace(*_, msg: string, site: string, trace: string, out: []u8): usize {
        if (_.silenced || _.no_deprecation) { return 0; }
        let h = depdHashKey(_.namespace, msg);
        let i: usize = 0;
        while (i < _.seen_count) {
            if (_.seen_hashes[i] == h) { return 0; }
            i = i + 1;
        }
        if (_.seen_count < MAX_SEEN_HASHES) {
            _.seen_hashes[_.seen_count] = h;
            _.seen_count = _.seen_count + 1;
        }
        _.emitted_count = _.emitted_count + 1;
        return formatDeprecationWithTrace(_.namespace, msg, site, trace, _.trace_deprecation, out);
    }

    // Alias for `warnOnce` matching npm `deprecate(msg)` ergonomics.
    pub fn deprecate(*_, msg: string, site: string, out: []u8): usize {
        return _.warnOnce(msg, site, out);
    }

    // Emits a deduplicated warning for a deprecated function/property name.
    // Falls back to `fn_name` when `msg` is empty.
    pub fn deprecateFn(*_, fn_name: string, msg: string, site: string, out: []u8): usize {
        const effective_msg = if (msg.len > 0) { msg } else { fn_name };
        return _.warnOnce(effective_msg, site, out);
    }

    pub fn reset(*_) {
        _.seen_count = 0;
        _.emitted_count = 0;
    }
}

/// Computes exact byte length produced by `formatDeprecation(namespace, msg, site, out)`.
pub fn formatDeprecationLen(namespace: string, msg: string, site: string): usize {
    let total: usize = namespace.len + 12 + msg.len; // " deprecated " = 12
    if (site.len > 0) {
        total = total + 4 + site.len; // " at " = 4
    }
    return total;
}

pub fn formatDeprecation(namespace: string, msg: string, site: string, out: []u8): usize {
    let p: usize = 0;
    p = copySlice(out, p, namespace);
    p = copySlice(out, p, " deprecated ");
    p = copySlice(out, p, msg);
    if (site.len > 0) {
        p = copySlice(out, p, " at ");
        p = copySlice(out, p, site);
    }
    return p;
}

/// Formats "<namespace> deprecated <msg>[ at <site>][\n<trace>]" into `out`.
pub fn formatDeprecationWithTrace(
    namespace: string,
    msg: string,
    site: string,
    trace: string,
    include_trace: bool,
    out: []u8
): usize {
    let p: usize = formatDeprecation(namespace, msg, site, out);
    if (include_trace && trace.len > 0) {
        p = copySlice(out, p, "\n");
        p = copySlice(out, p, trace);
    }
    return p;
}

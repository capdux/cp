// Source: Absorbed from /data/dev/bun/src/http/http.cp | Synced: 2026-11-22T12:17:21Z (RFC 9111 §4.2 HTTP Cache Freshness & Age Calculator)
// Pure-CP zero-allocation RFC 9111 §4.2 HTTP cache freshness lifetime, current age, and stale-while-revalidate evaluator.

export enum CacheFreshness {
    fresh,
    stale_while_revalidate,
    stale,
}

export struct FreshnessInput {
    is_shared_cache: bool,
    max_age_sec: u64?,
    s_maxage_sec: u64?,
    stale_while_revalidate_sec: u64?,
    stale_if_error_sec: u64?,
    expires_unix_sec: i64?,
    date_unix_sec: i64?,
    last_modified_unix_sec: i64?,
    age_header_sec: u64?,
    request_time_sec: i64,
    response_time_sec: i64,

    pub fn empty(is_shared_cache: bool, request_time_sec: i64, response_time_sec: i64): FreshnessInput {
        return FreshnessInput {
            is_shared_cache: is_shared_cache,
            max_age_sec: null,
            s_maxage_sec: null,
            stale_while_revalidate_sec: null,
            stale_if_error_sec: null,
            expires_unix_sec: null,
            date_unix_sec: null,
            last_modified_unix_sec: null,
            age_header_sec: null,
            request_time_sec: request_time_sec,
            response_time_sec: response_time_sec,
        };
    }

    /// RFC 9111 §4.2.1 freshness lifetime in seconds:
    /// 1. `s-maxage` (if shared cache)
    /// 2. `max-age`
    /// 3. `Expires - Date` (clamped to >= 0)
    /// 4. Heuristic 10% of `(Date - Last-Modified)` per RFC 9111 §4.2.2
    pub fn freshnessLifetimeSec(): u64 {
        if (_.is_shared_cache) {
            if (sm := _.s_maxage_sec) return sm;
        }
        if (ma := _.max_age_sec) return ma;
        if (exp := _.expires_unix_sec) {
            const ref_date = _.date_unix_sec ?? _.response_time_sec;
            if (exp <= ref_date) return 0;
            return @as(u64, exp - ref_date);
        }
        if (lm := _.last_modified_unix_sec) {
            const ref_date = _.date_unix_sec ?? _.response_time_sec;
            if (ref_date > lm) {
                return @as(u64, ref_date - lm) / 10;
            }
        }
        return 0;
    }

    /// RFC 9111 §4.2.3 current age in seconds at `now_unix_sec`.
    pub fn currentAgeSec(now_unix_sec: i64): u64 {
        let apparent_age: u64 = 0;
        if (d := _.date_unix_sec) {
            if (_.response_time_sec > d) {
                apparent_age = @as(u64, _.response_time_sec - d);
            }
        }
        let response_delay: u64 = 0;
        if (_.response_time_sec > _.request_time_sec) {
            response_delay = @as(u64, _.response_time_sec - _.request_time_sec);
        }
        const age_val = _.age_header_sec ?? 0;
        const corrected_age_value = age_val + response_delay;
        const corrected_initial_age = if (apparent_age > corrected_age_value) apparent_age else corrected_age_value;

        let resident_time: u64 = 0;
        if (now_unix_sec > _.response_time_sec) {
            resident_time = @as(u64, now_unix_sec - _.response_time_sec);
        }
        return corrected_initial_age + resident_time;
    }

    /// Evaluate freshness status at `now_unix_sec`.
    pub fn evaluate(now_unix_sec: i64): CacheFreshness {
        const lifetime = _.freshnessLifetimeSec();
        const age = _.currentAgeSec(now_unix_sec);
        if (age < lifetime) return CacheFreshness.fresh;
        if (swr := _.stale_while_revalidate_sec) {
            if (age < lifetime + swr) return CacheFreshness.stale_while_revalidate;
        }
        return CacheFreshness.stale;
    }

    /// Remaining fresh seconds at `now_unix_sec` (0 if stale).
    pub fn remainingFreshSec(now_unix_sec: i64): u64 {
        const lifetime = _.freshnessLifetimeSec();
        const age = _.currentAgeSec(now_unix_sec);
        if (age >= lifetime) return 0;
        return lifetime - age;
    }

    /// True if RFC 5861 `stale-if-error` permits serving this stale entry on origin 5xx error at `now_unix_sec`.
    pub fn canServeStaleOnError(now_unix_sec: i64): bool {
        const lifetime = _.freshnessLifetimeSec();
        const age = _.currentAgeSec(now_unix_sec);
        if (age < lifetime) return true;
        if (sie := _.stale_if_error_sec) {
            return age < lifetime + sie;
        }
        return false;
    }
}

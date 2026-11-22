// Source: Absorbed from /data/dev/bun/src/bake/DevServer/RouteBundle.cp | Synced: 2026-11-22T17:13:24Z
// Pure-CP zero-allocation DevServer RouteBundle lifecycle state machine & registry:
// tracks Framework (SSR/fullstack) and HTML (SPA) route bundles, 6-state bundling lifecycle,
// client_script_generation staleness detection, active HMR viewer ref-counting,
// source-map key derivation, and memory cost accounting.

pub const ROUTE_BUNDLE_MAX_COUNT: usize = 128;

pub error RouteBundleError {
    CapacityExceeded,
    InvalidRouteIndex,
    InvalidStateTransition,
    ViewerUnderflow,
    BufferTooSmall,
}

pub enum RouteBundleKind: u8 {
    Framework = 0,
    Html = 1,
}

/// 6-state bundling lifecycle mirrored from Bun `RouteBundle.State`.
pub enum RouteBundleState: u8 {
    Unqueued = 0,
    Bundling = 1,
    DeferredToNextBundle = 2,
    PossibleBundlingFailures = 3,
    EvaluationFailure = 4,
    Loaded = 5,

    fn isReady(): bool {
        return _ == .Loaded;
    }

    fn isFailed(): bool {
        return _ == .PossibleBundlingFailures || _ == .EvaluationFailure;
    }

    fn isPending(): bool {
        return _ == .Unqueued || _ == .Bundling || _ == .DeferredToNextBundle;
    }
}

pub struct RouteBundleEntry {
    id: u32 = 0,
    kind: RouteBundleKind = .Framework,
    state: RouteBundleState = .Unqueued,
    route_pattern: []const u8 = "",
    client_script_generation: u32 = 1,
    active_viewers: u32 = 0,
    has_client_bundle: bool = false,
    client_bundle_bytes: usize = 0,
    bundled_html_bytes: usize = 0,
    cached_response_bytes: usize = 0,
    script_injection_offset: u32? = null,
    failure_count: u32 = 0,

    /// Derives a 64-bit source-map lookup key combining `client_script_generation` and route `id`.
    /// Mirrors Bun `RouteBundle.sourceMapId()`.
    fn sourceMapKey(): u64 {
        return (@as(u64, _.client_script_generation) << 32) | @as(u64, _.id);
    }

    fn hasActiveViewers(): bool {
        return _.active_viewers > 0;
    }

    /// Returns `true` iff a browser request's generation matches the live client bundle generation.
    /// A mismatch instructs the dev server to serve a full-page reload stub.
    fn isGenerationCurrent(gen: u32): bool {
        return _.has_client_bundle && _.client_script_generation == gen;
    }

    /// Mirrors Bun `RouteBundle.memoryCost()` across client bundle & HTML response buffers.
    fn memoryCost(): usize {
        return _.client_bundle_bytes + _.bundled_html_bytes + _.cached_response_bytes;
    }

    /// Formats `/_bun/client/<id>-<8-lower-hex-gen>.js` into caller-provided `out`.
    fn formatClientBundleUrl(out: []u8): []const u8 ! RouteBundleError {
        const prefix = "/_bun/client/";
        let id_digits: [10]u8 = [0; 10];
        let id_len: usize = 0;
        if (_.id == 0) {
            id_digits[0] = '0';
            id_len = 1;
        } else {
            let v = _.id;
            while (v > 0) {
                id_digits[id_len] = @as(u8, '0' + (v % 10));
                v /= 10;
                id_len += 1;
            }
        }
        const total = prefix.len + id_len + 1 + 8 + 3;
        if (total > out.len) return err.BufferTooSmall;

        let pos: usize = 0;
        let i: usize = 0;
        while (i < prefix.len) {
            out[pos] = prefix[i];
            pos += 1;
            i += 1;
        }
        // Write reversed id_digits
        let d = id_len;
        while (d > 0) {
            d -= 1;
            out[pos] = id_digits[d];
            pos += 1;
        }
        out[pos] = '-';
        pos += 1;

        const hex = "0123456789abcdef";
        let shift: u5 = 28;
        let h: usize = 0;
        while (h < 8) {
            const nib: usize = @as(usize, (_.client_script_generation >> shift) & 0xF);
            out[pos] = hex[nib];
            pos += 1;
            if (shift >= 4) shift -= 4;
            h += 1;
        }
        out[pos] = '.';
        out[pos + 1] = 'j';
        out[pos + 2] = 's';
        pos += 3;
        return out[0..pos];
    }
}

/// Fixed-capacity zero-allocation registry of `RouteBundleEntry` state machines.
pub struct RouteBundleRegistry {
    entries: [ROUTE_BUNDLE_MAX_COUNT]RouteBundleEntry = [RouteBundleEntry{}; ROUTE_BUNDLE_MAX_COUNT],
    entry_count: usize = 0,

    static fn init(): RouteBundleRegistry {
        return RouteBundleRegistry{};
    }

    fn count(): usize {
        return _.entry_count;
    }

    fn registerFramework(*_, pattern: []const u8, initial_gen: u32): u32 ! RouteBundleError {
        return _.registerInternal(.Framework, pattern, initial_gen);
    }

    fn registerHtml(*_, pattern: []const u8, initial_gen: u32): u32 ! RouteBundleError {
        return _.registerInternal(.Html, pattern, initial_gen);
    }

    fn get(id: u32): RouteBundleEntry ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        return _.entries[idx];
    }

    fn findByPattern(pattern: []const u8): u32? {
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].route_pattern == pattern) {
                return _.entries[i].id;
            }
            i += 1;
        }
        return null;
    }

    /// Transitions route into `.Bundling`. Rejects re-entering `.Bundling` while already bundling.
    fn beginBundling(*_, id: u32): RouteBundleState ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        if (_.entries[idx].state == .Bundling) return err.InvalidStateTransition;
        _.entries[idx].state = .Bundling;
        return .Bundling;
    }

    /// Marks a bundling route as `.Loaded` and records bundle memory sizes.
    fn markLoaded(
        *_,
        id: u32,
        client_bytes: usize,
        html_bytes: usize,
        cached_resp_bytes: usize,
        injection_offset: u32?,
    ): RouteBundleState ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        if (_.entries[idx].state != .Bundling) return err.InvalidStateTransition;
        _.entries[idx].state = .Loaded;
        _.entries[idx].has_client_bundle = client_bytes > 0;
        _.entries[idx].client_bundle_bytes = client_bytes;
        _.entries[idx].bundled_html_bytes = html_bytes;
        _.entries[idx].cached_response_bytes = cached_resp_bytes;
        _.entries[idx].script_injection_offset = injection_offset;
        return .Loaded;
    }

    fn markBundlingFailure(*_, id: u32): RouteBundleState ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        _.entries[idx].state = .PossibleBundlingFailures;
        _.entries[idx].failure_count += 1;
        return .PossibleBundlingFailures;
    }

    fn markEvaluationFailure(*_, id: u32): RouteBundleState ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        _.entries[idx].state = .EvaluationFailure;
        _.entries[idx].failure_count += 1;
        return .EvaluationFailure;
    }

    fn deferToNextBundle(*_, id: u32): RouteBundleState ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        _.entries[idx].state = .DeferredToNextBundle;
        return .DeferredToNextBundle;
    }

    /// Invalidates the client bundle and HTML cached response, rotating `client_script_generation`
    /// deterministically (non-zero) so stale browser fetches trigger an automatic reload.
    /// Returns the new generation.
    fn invalidateClientBundle(*_, id: u32): u32 ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        _.entries[idx].has_client_bundle = false;
        _.entries[idx].client_bundle_bytes = 0;
        if (_.entries[idx].kind == .Html) {
            _.entries[idx].cached_response_bytes = 0;
        }
        let next_gen = _.entries[idx].client_script_generation +% 1;
        if (next_gen == 0) next_gen = 1;
        _.entries[idx].client_script_generation = next_gen;
        return next_gen;
    }

    /// Increments the active HMR viewer count on this route and returns the updated count.
    fn addViewer(*_, id: u32): u32 ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        _.entries[idx].active_viewers += 1;
        return _.entries[idx].active_viewers;
    }

    /// Decrements the active HMR viewer count on this route; prevents underflow.
    fn removeViewer(*_, id: u32): u32 ! RouteBundleError {
        const idx = @as(usize, id);
        if (idx >= _.entry_count) return err.InvalidRouteIndex;
        if (_.entries[idx].active_viewers == 0) return err.ViewerUnderflow;
        _.entries[idx].active_viewers -= 1;
        return _.entries[idx].active_viewers;
    }

    /// Returns how many routes currently have at least one connected HMR viewer.
    fn activeRouteCount(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            if (_.entries[i].active_viewers > 0) total += 1;
            i += 1;
        }
        return total;
    }

    fn totalMemoryCost(): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.entry_count) {
            total += _.entries[i].memoryCost();
            i += 1;
        }
        return total;
    }

    // --- Internal helper ---

    fn registerInternal(*_, kind: RouteBundleKind, pattern: []const u8, initial_gen: u32): u32 ! RouteBundleError {
        if (_.entry_count >= ROUTE_BUNDLE_MAX_COUNT) {
            return err.CapacityExceeded;
        }
        const id: u32 = @as(u32, _.entry_count);
        const gen: u32 = if (initial_gen == 0) 1 else initial_gen;
        _.entries[_.entry_count] = RouteBundleEntry{
            id: id,
            kind: kind,
            state: .Unqueued,
            route_pattern: pattern,
            client_script_generation: gen,
        };
        _.entry_count += 1;
        return id;
    }
}

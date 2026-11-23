// Source: Absorbed from /data/dev/bun/src/cli/update_interactive_command.cp | Synced: 2026-11-21T23:04:12Z
// ============================================================================
// lib/cli/interactiveUpdate.cp — Interactive Dependency Upgrade TUI & Diff Engine
// ============================================================================
// Pure-CP, zero-heap, caller-slice-driven interactive `cap update -i` engine:
//   1. SemVer upgrade risk classifier & segment-diff highlighter
//      (`classifySemverUpgrade`, `formatVersionDiffHighlight`) distinguishing
//      `UpToDate`, `Patch` (green), `Minor` (yellow), `Major` (red), and
//      `Prerelease` (magenta), highlighting only the changed version suffix;
//   2. Interactive TUI multi-select state machine (`InteractiveUpdateState`)
//      supporting dependency groups, keyboard events (`Up`/`Down`/`PageUp`/
//      `PageDown`/`Space`/`a`/`n`/`l`/`Enter`/`Esc`), bounded viewport scrolling,
//      and `cap:simd`-accelerated case-insensitive search filtering;
//   3. Aligned terminal table renderer (`renderInteractiveUpdateTable`) with
//      ANSI color and plain-text fallback modes plus selected upgrade export.

import { Simd } from "cap:simd";

/// Default visible row count in the interactive viewport.
pub const INTERACTIVE_UPDATE_DEFAULT_VIEWPORT: usize = 20;

/// Maximum length of the live interactive search filter query.
pub const INTERACTIVE_UPDATE_MAX_FILTER_LEN: usize = 64;

/// Error set for interactive update state initialization, export, and rendering.
pub err InteractiveUpdateError {
    SliceCapacityExceeded,
    BufferTooSmall,
    InvalidViewportHeight,
}

// ============================================================================
// Internal Zero-Alloc Byte, SIMD Substring, and Formatting Helpers
// ============================================================================

const IU_ANSI_RESET: []const u8 = "\x1b[0m";
const IU_ANSI_BOLD: []const u8 = "\x1b[1m";
const IU_ANSI_DIM: []const u8 = "\x1b[90m";
const IU_ANSI_GREEN: []const u8 = "\x1b[32m";
const IU_ANSI_YELLOW: []const u8 = "\x1b[33m";
const IU_ANSI_RED: []const u8 = "\x1b[31m";
const IU_ANSI_MAGENTA: []const u8 = "\x1b[35m";
const IU_ANSI_CYAN: []const u8 = "\x1b[36m";

fn iuAppendByte(out: []u8, pos: *usize, b: u8)! {
    if (pos.* >= out.len) return err.BufferTooSmall;
    out[pos.*] = b;
    pos.* += 1;
}

fn iuAppendBytes(out: []u8, pos: *usize, src: []const u8)! {
    if (src.len > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < src.len) {
        out[pos.* + i] = src[i];
        i += 1;
    }
    pos.* += src.len;
}

fn iuAppendSpaces(out: []u8, pos: *usize, count: usize)! {
    if (count > out.len - pos.*) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < count) {
        out[pos.* + i] = ' ';
        i += 1;
    }
    pos.* += count;
}

fn iuAppendU64(out: []u8, pos: *usize, val: u64)! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        iuAppendByte(out, pos, '0')?;
        return;
    }
    while (v > 0) {
        const d: u8 = @as(u8, @intCast(v % 10));
        tmp[n] = @as(u8, '0') + d;
        n += 1;
        v = v / 10;
    }
    while (n > 0) {
        n -= 1;
        iuAppendByte(out, pos, tmp[n])?;
    }
}

fn iuBytesEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

/// Case-insensitive substring match using `cap:simd` `Simd.toLowerAscii`.
fn iuContainsIgnoreCaseSimd(haystack: []const u8, needle_lower: []const u8): bool {
    if (needle_lower.len == 0) return true;
    if (needle_lower.len > haystack.len) return false;

    let low_buf: [128]u8 = [0; 128];
    const hlen = if (haystack.len <= low_buf.len) haystack.len else low_buf.len;
    * = Simd.toLowerAscii(haystack[0..hlen], low_buf[0..hlen]);
    if (needle_lower.len > hlen) return false;

    const first_b = needle_lower[0];
    let pos: usize = 0;
    while (pos + needle_lower.len <= hlen) {
        const rel_opt = Simd.indexOfByte(low_buf[pos..hlen], first_b);
        let rel: usize = 0;
        if (v := rel_opt) {
            rel = v;
        } else {
            return false;
        }
        const cand = pos + rel;
        if (cand + needle_lower.len > hlen) return false;
        if (iuBytesEql(low_buf[cand..cand + needle_lower.len], needle_lower)) {
            return true;
        }
        pos = cand + 1;
    }
    return false;
}

// ============================================================================
// Section 1: SemVer Upgrade Risk Classification & Diff Highlighting
// ============================================================================

/// Parsed SemVer major.minor.patch triple with prerelease indicator.
pub struct IuSemverTriple {
    major: u32 = 0,
    minor: u32 = 0,
    patch: u32 = 0,
    has_prerelease: bool = false,
    valid: bool = false,
}

fn iuStripVersionPrefix(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c == '^' || c == '~' || c == '=' || c == 'v' || c == 'V' || c == ' ') {
            i += 1;
        } else {
            break;
        }
    }
    return s[i..];
}

/// Parses a SemVer string (tolerating leading `^`, `~`, `=`, `v`) into `IuSemverTriple`.
pub fn parseIuSemver(raw: []const u8): IuSemverTriple {
    const s = iuStripVersionPrefix(raw);
    if (s.len == 0) return IuSemverTriple{};

    let parts: [3]u32 = [0; 3];
    let part_idx: usize = 0;
    let i: usize = 0;
    let has_pre = false;

    while (part_idx < 3 && i < s.len) {
        let val: u64 = 0;
        let digits: usize = 0;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            digits += 1;
            if (digits > 9) return IuSemverTriple{};
            val = val * 10 + @as(u64, s[i] - '0');
            i += 1;
        }
        if (digits == 0) return IuSemverTriple{};
        parts[part_idx] = @as(u32, @intCast(val));
        part_idx += 1;

        if (i < s.len) {
            if (s[i] == '.') {
                i += 1;
            } else if (s[i] == '-') {
                has_pre = true;
                break;
            } else if (s[i] == '+') {
                break;
            } else {
                return IuSemverTriple{};
            }
        }
    }

    if (part_idx == 0) return IuSemverTriple{};
    return IuSemverTriple{
        major: parts[0],
        minor: parts[1],
        patch: parts[2],
        has_prerelease: has_pre,
        valid: true,
    };
}

/// SemVer upgrade severity classification.
pub enum IuUpgradeKind: u8 {
    UpToDate = 0,
    Patch = 1,
    Minor = 2,
    Major = 3,
    Prerelease = 4,
}

/// Detailed classification and byte-offset diff anchor for a version upgrade.
pub struct IuUpgradeClassification {
    kind: IuUpgradeKind = IuUpgradeKind.UpToDate,
    diff_start_offset: usize = 0,
    is_breaking: bool = false,
}

/// Returns the ANSI foreground escape sequence for `kind`.
pub fn iuUpgradeAnsiColor(kind: IuUpgradeKind): []const u8 {
    if (kind == IuUpgradeKind.Patch) return IU_ANSI_GREEN;
    if (kind == IuUpgradeKind.Minor) return IU_ANSI_YELLOW;
    if (kind == IuUpgradeKind.Major) return IU_ANSI_RED;
    if (kind == IuUpgradeKind.Prerelease) return IU_ANSI_MAGENTA;
    return IU_ANSI_DIM;
}

/// Classifies the SemVer delta between `current` and `candidate`, computing the
/// exact byte offset in `candidate` where the changed version segment begins
/// (e.g., `1.2.3 -> 1.4.0` yields `Minor` with `diff_start_offset == 2` so `4.0`
/// is highlighted).
pub fn classifySemverUpgrade(current: []const u8, candidate: []const u8): IuUpgradeClassification {
    if (iuBytesEql(current, candidate)) {
        return IuUpgradeClassification{
            kind: IuUpgradeKind.UpToDate,
            diff_start_offset: candidate.len,
            is_breaking: false,
        };
    }

    const cur = parseIuSemver(current);
    const nxt = parseIuSemver(candidate);

    if (!cur.valid || !nxt.valid) {
        return IuUpgradeClassification{
            kind: IuUpgradeKind.Major,
            diff_start_offset: 0,
            is_breaking: true,
        };
    }

    // Locate dot boundaries in `candidate` for precise segment highlighting
    let prefix_skip: usize = 0;
    while (prefix_skip < candidate.len) {
        const ch = candidate[prefix_skip];
        if (ch == '^' || ch == '~' || ch == '=' || ch == 'v' || ch == 'V' || ch == ' ') {
            prefix_skip += 1;
        } else {
            break;
        }
    }

    let first_dot: usize = candidate.len;
    let second_dot: usize = candidate.len;
    let dash_pos: usize = candidate.len;
    let scan = prefix_skip;
    while (scan < candidate.len) {
        const ch = candidate[scan];
        if (ch == '.') {
            if (first_dot == candidate.len) {
                first_dot = scan;
            } else if (second_dot == candidate.len) {
                second_dot = scan;
            }
        } else if (ch == '-' && dash_pos == candidate.len) {
            dash_pos = scan;
        }
        scan += 1;
    }

    if (nxt.major != cur.major) {
        return IuUpgradeClassification{
            kind: IuUpgradeKind.Major,
            diff_start_offset: prefix_skip,
            is_breaking: true,
        };
    }
    if (nxt.minor != cur.minor) {
        const off = if (first_dot + 1 <= candidate.len) first_dot + 1 else prefix_skip;
        return IuUpgradeClassification{
            kind: IuUpgradeKind.Minor,
            diff_start_offset: off,
            is_breaking: cur.major == 0,
        };
    }
    if (nxt.patch != cur.patch) {
        const off = if (second_dot + 1 <= candidate.len) second_dot + 1 else prefix_skip;
        return IuUpgradeClassification{
            kind: IuUpgradeKind.Patch,
            diff_start_offset: off,
            is_breaking: false,
        };
    }
    if (nxt.has_prerelease || cur.has_prerelease) {
        const off = if (dash_pos < candidate.len) dash_pos else prefix_skip;
        return IuUpgradeClassification{
            kind: IuUpgradeKind.Prerelease,
            diff_start_offset: off,
            is_breaking: false,
        };
    }

    return IuUpgradeClassification{
        kind: IuUpgradeKind.UpToDate,
        diff_start_offset: candidate.len,
        is_breaking: false,
    };
}

/// Formats `target` into `out_buf`, optionally wrapping only the changed suffix
/// starting at `classifySemverUpgrade(current, target).diff_start_offset` with
/// the corresponding ANSI severity color.
pub fn formatVersionDiffHighlight(
    current: []const u8,
    target: []const u8,
    ansi_colors: bool,
    out_buf: []u8,
): InteractiveUpdateError!usize {
    let pos: usize = 0;
    if (!ansi_colors) {
        iuAppendBytes(out_buf, &pos, target)?;
        return pos;
    }

    const cls = classifySemverUpgrade(current, target);
    if (cls.kind == IuUpgradeKind.UpToDate || cls.diff_start_offset >= target.len) {
        iuAppendBytes(out_buf, &pos, IU_ANSI_DIM)?;
        iuAppendBytes(out_buf, &pos, target)?;
        iuAppendBytes(out_buf, &pos, IU_ANSI_RESET)?;
        return pos;
    }

    if (cls.diff_start_offset > 0) {
        iuAppendBytes(out_buf, &pos, target[0..cls.diff_start_offset])?;
    }
    iuAppendBytes(out_buf, &pos, iuUpgradeAnsiColor(cls.kind))?;
    iuAppendBytes(out_buf, &pos, target[cls.diff_start_offset..])?;
    iuAppendBytes(out_buf, &pos, IU_ANSI_RESET)?;
    return pos;
}

// ============================================================================
// Section 2: Interactive Multi-Select State Machine (`InteractiveUpdateState`)
// ============================================================================

/// Dependency section grouping in `cap.json`.
pub enum IuDepGroup: u8 {
    Dependencies = 0,
    DevDependencies = 1,
    OptionalDependencies = 2,
    PeerDependencies = 3,
}

/// Returns the canonical `cap.json` field name for `group`.
pub fn iuDepGroupLabel(group: IuDepGroup): []const u8 {
    if (group == IuDepGroup.Dependencies) return "dependencies";
    if (group == IuDepGroup.DevDependencies) return "devDependencies";
    if (group == IuDepGroup.OptionalDependencies) return "optionalDependencies";
    return "peerDependencies";
}

/// Single outdated dependency row managed by `InteractiveUpdateState`.
pub struct IuPackageEntry {
    name: []const u8 = "",
    workspace: []const u8 = "",
    group: IuDepGroup = IuDepGroup.Dependencies,
    current_version: []const u8 = "",
    target_version: []const u8 = "",
    latest_version: []const u8 = "",
    selected: bool = false,
    use_latest: bool = false,

    static fn empty(): IuPackageEntry {
        return IuPackageEntry{};
    }

    /// Returns the currently chosen destination version (`latest_version` if `use_latest` else `target_version`).
    fn chosenVersion(): []const u8 {
        if (_.use_latest && _.latest_version.len > 0) return _.latest_version;
        return _.target_version;
    }
}

/// Keyboard navigation & selection event fed into `InteractiveUpdateState.handleKey`.
pub enum IuKeyEvent: u8 {
    Up = 0,
    Down = 1,
    PageUp = 2,
    PageDown = 3,
    Home = 4,
    End = 5,
    ToggleSelect = 6,
    SelectAll = 7,
    SelectNone = 8,
    ToggleLatest = 9,
    Confirm = 10,
    Cancel = 11,
    ClearFilter = 12,
}

/// Terminal session outcome for `InteractiveUpdateState`.
pub enum IuSessionOutcome: u8 {
    Active = 0,
    Confirmed = 1,
    Cancelled = 2,
}

/// Final selected upgrade action exported when the user confirms.
pub struct IuSelectedUpgrade {
    entry_index: usize = 0,
    name: []const u8 = "",
    workspace: []const u8 = "",
    group: IuDepGroup = IuDepGroup.Dependencies,
    from_version: []const u8 = "",
    to_version: []const u8 = "",
    upgrade_kind: IuUpgradeKind = IuUpgradeKind.UpToDate,

    static fn empty(): IuSelectedUpgrade {
        return IuSelectedUpgrade{};
    }
}

/// Zero-heap, caller-slice-backed interactive dependency upgrade TUI state machine.
pub struct InteractiveUpdateState {
    entries: []IuPackageEntry,
    visible_indices: []usize,
    visible_count: usize = 0,
    filter_buf: [INTERACTIVE_UPDATE_MAX_FILTER_LEN]u8 = [0; INTERACTIVE_UPDATE_MAX_FILTER_LEN],
    filter_len: usize = 0,
    cursor: usize = 0,
    viewport_start: usize = 0,
    viewport_height: usize = INTERACTIVE_UPDATE_DEFAULT_VIEWPORT,
    outcome: IuSessionOutcome = IuSessionOutcome.Active,

    static fn init(
        entries: []IuPackageEntry,
        visible_indices: []usize,
        viewport_height: usize,
    ): InteractiveUpdateError!InteractiveUpdateState {
        if (visible_indices.len < entries.len) return err.SliceCapacityExceeded;
        if (viewport_height == 0) return err.InvalidViewportHeight;

        let st = InteractiveUpdateState{
            entries: entries,
            visible_indices: visible_indices,
            visible_count: 0,
            viewport_height: viewport_height,
        };
        st.rebuildVisible();
        return st;
    }

    fn clampViewport(*_) {
        if (_.visible_count == 0) {
            _.cursor = 0;
            _.viewport_start = 0;
            return;
        }
        if (_.cursor >= _.visible_count) {
            _.cursor = _.visible_count - 1;
        }
        if (_.cursor < _.viewport_start) {
            _.viewport_start = _.cursor;
        } else if (_.cursor >= _.viewport_start + _.viewport_height) {
            _.viewport_start = _.cursor - _.viewport_height + 1;
        }
        if (_.visible_count <= _.viewport_height) {
            _.viewport_start = 0;
        } else if (_.viewport_start + _.viewport_height > _.visible_count) {
            _.viewport_start = _.visible_count - _.viewport_height;
        }
    }

    fn rebuildVisible(*_) {
        let lower_q: [INTERACTIVE_UPDATE_MAX_FILTER_LEN]u8 = [0; INTERACTIVE_UPDATE_MAX_FILTER_LEN];
        if (_.filter_len > 0) {
            * = Simd.toLowerAscii(_.filter_buf[0.._.filter_len], lower_q[0.._.filter_len]);
        }
        const q = lower_q[0.._.filter_len];

        // Order visible entries stably by dependency group (prod -> dev -> optional -> peer)
        const groups: [4]IuDepGroup = [
            IuDepGroup.Dependencies,
            IuDepGroup.DevDependencies,
            IuDepGroup.OptionalDependencies,
            IuDepGroup.PeerDependencies,
        ];
        let count: usize = 0;
        let g_idx: usize = 0;
        while (g_idx < 4) {
            const target_g = groups[g_idx];
            let i: usize = 0;
            while (i < _.entries.len) {
                const e = _.entries[i];
                if (e.group == target_g) {
                    const matched = (q.len == 0) ||
                        iuContainsIgnoreCaseSimd(e.name, q) ||
                        iuContainsIgnoreCaseSimd(e.workspace, q);
                    if (matched && count < _.visible_indices.len) {
                        _.visible_indices[count] = i;
                        count += 1;
                    }
                }
                i += 1;
            }
            g_idx += 1;
        }

        _.visible_count = count;
        _.clampViewport();
    }

    /// Sets the live search filter query and updates the visible list and viewport.
    fn setFilter(*_, query: []const u8) {
        const n = if (query.len <= INTERACTIVE_UPDATE_MAX_FILTER_LEN) query.len else INTERACTIVE_UPDATE_MAX_FILTER_LEN;
        let i: usize = 0;
        while (i < n) {
            _.filter_buf[i] = query[i];
            i += 1;
        }
        _.filter_len = n;
        _.rebuildVisible();
    }

    /// Appends one printable ASCII character to the live search filter.
    fn pushFilterChar(*_, ch: u8) {
        if (_.outcome != IuSessionOutcome.Active) return;
        if (_.filter_len < INTERACTIVE_UPDATE_MAX_FILTER_LEN && ch >= 0x20 && ch < 0x7F) {
            _.filter_buf[_.filter_len] = ch;
            _.filter_len += 1;
            _.rebuildVisible();
        }
    }

    /// Deletes the last character from the live search filter.
    fn popFilterChar(*_) {
        if (_.outcome != IuSessionOutcome.Active) return;
        if (_.filter_len > 0) {
            _.filter_len -= 1;
            _.rebuildVisible();
        }
    }

    /// Applies one keyboard event to the interactive TUI state machine.
    fn handleKey(*_, ev: IuKeyEvent) {
        if (_.outcome != IuSessionOutcome.Active) return;

        if (ev == IuKeyEvent.Confirm) {
            _.outcome = IuSessionOutcome.Confirmed;
            return;
        }
        if (ev == IuKeyEvent.Cancel) {
            _.outcome = IuSessionOutcome.Cancelled;
            return;
        }
        if (ev == IuKeyEvent.ClearFilter) {
            _.filter_len = 0;
            _.rebuildVisible();
            return;
        }

        if (_.visible_count == 0) {
            _.clampViewport();
            return;
        }

        if (ev == IuKeyEvent.Up) {
            if (_.cursor > 0) {
                _.cursor -= 1;
            } else {
                _.cursor = _.visible_count - 1;
            }
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.Down) {
            if (_.cursor + 1 < _.visible_count) {
                _.cursor += 1;
            } else {
                _.cursor = 0;
            }
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.PageUp) {
            _.cursor = if (_.cursor >= _.viewport_height) _.cursor - _.viewport_height else 0;
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.PageDown) {
            const nxt = _.cursor + _.viewport_height;
            _.cursor = if (nxt < _.visible_count) nxt else _.visible_count - 1;
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.Home) {
            _.cursor = 0;
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.End) {
            _.cursor = _.visible_count - 1;
            _.clampViewport();
            return;
        }
        if (ev == IuKeyEvent.ToggleSelect) {
            const eidx = _.visible_indices[_.cursor];
            const prev = _.entries[eidx];
            _.entries[eidx] = IuPackageEntry{
                name: prev.name,
                workspace: prev.workspace,
                group: prev.group,
                current_version: prev.current_version,
                target_version: prev.target_version,
                latest_version: prev.latest_version,
                selected: !prev.selected,
                use_latest: prev.use_latest,
            };
            return;
        }
        if (ev == IuKeyEvent.ToggleLatest) {
            const eidx = _.visible_indices[_.cursor];
            const prev = _.entries[eidx];
            _.entries[eidx] = IuPackageEntry{
                name: prev.name,
                workspace: prev.workspace,
                group: prev.group,
                current_version: prev.current_version,
                target_version: prev.target_version,
                latest_version: prev.latest_version,
                selected: prev.selected,
                use_latest: !prev.use_latest,
            };
            return;
        }
        if (ev == IuKeyEvent.SelectAll || ev == IuKeyEvent.SelectNone) {
            const flag = ev == IuKeyEvent.SelectAll;
            let i: usize = 0;
            while (i < _.visible_count) {
                const eidx = _.visible_indices[i];
                const prev = _.entries[eidx];
                _.entries[eidx] = IuPackageEntry{
                    name: prev.name,
                    workspace: prev.workspace,
                    group: prev.group,
                    current_version: prev.current_version,
                    target_version: prev.target_version,
                    latest_version: prev.latest_version,
                    selected: flag,
                    use_latest: prev.use_latest,
                };
                i += 1;
            }
            return;
        }
    }

    /// Returns total number of currently selected packages across all entries.
    fn selectedCount(): usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            if (_.entries[i].selected) count += 1;
            i += 1;
        }
        return count;
    }

    /// Exports all currently selected upgrades into caller-provided `out_upgrades`.
    fn exportSelectedUpgrades(out_upgrades: []IuSelectedUpgrade): InteractiveUpdateError!usize {
        let count: usize = 0;
        let i: usize = 0;
        while (i < _.entries.len) {
            const e = _.entries[i];
            if (e.selected) {
                if (count >= out_upgrades.len) return err.SliceCapacityExceeded;
                const dest = e.chosenVersion();
                const cls = classifySemverUpgrade(e.current_version, dest);
                out_upgrades[count] = IuSelectedUpgrade{
                    entry_index: i,
                    name: e.name,
                    workspace: e.workspace,
                    group: e.group,
                    from_version: e.current_version,
                    to_version: dest,
                    upgrade_kind: cls.kind,
                };
                count += 1;
            }
            i += 1;
        }
        return count;
    }
}

// ============================================================================
// Section 3: Aligned Terminal Table Renderer (`renderInteractiveUpdateTable`)
// ============================================================================

/// Rendering configuration for `renderInteractiveUpdateTable`.
pub struct IuRenderOptions {
    ansi_colors: bool = true,
    show_workspace: bool = false,
    show_footer_help: bool = true,

    static fn defaults(): IuRenderOptions {
        return IuRenderOptions{};
    }
}

/// Renders the current interactive update viewport as an aligned terminal table
/// (with optional ANSI color diffing or clean plain-text fallback) into `out_buf`.
pub fn renderInteractiveUpdateTable(
    state: InteractiveUpdateState,
    opts: IuRenderOptions,
    out_buf: []u8,
): InteractiveUpdateError!usize {
    let pos: usize = 0;

    // Compute dynamic column widths across visible rows
    let w_name: usize = 7; // "Package"
    let w_cur: usize = 7;  // "Current"
    let w_tgt: usize = 6;  // "Target"
    let w_lat: usize = 6;  // "Latest"
    let w_ws: usize = 9;   // "Workspace"

    let i: usize = 0;
    while (i < state.visible_count) {
        const e = state.entries[state.visible_indices[i]];
        if (e.name.len > w_name) w_name = e.name.len;
        if (e.current_version.len > w_cur) w_cur = e.current_version.len;
        if (e.target_version.len > w_tgt) w_tgt = e.target_version.len;
        if (e.latest_version.len > w_lat) w_lat = e.latest_version.len;
        if (e.workspace.len > w_ws) w_ws = e.workspace.len;
        i += 1;
    }

    // Header & active filter banner
    if (opts.ansi_colors) iuAppendBytes(out_buf, &pos, IU_ANSI_BOLD)?;
    iuAppendBytes(out_buf, &pos, "cap update --interactive (")?;
    iuAppendU64(out_buf, &pos, @as(u64, state.selectedCount()))?;
    iuAppendByte(out_buf, &pos, '/')?;
    iuAppendU64(out_buf, &pos, @as(u64, state.entries.len))?;
    iuAppendBytes(out_buf, &pos, " selected)")?;
    if (opts.ansi_colors) iuAppendBytes(out_buf, &pos, IU_ANSI_RESET)?;
    if (state.filter_len > 0) {
        iuAppendBytes(out_buf, &pos, "  filter: \"")?;
        iuAppendBytes(out_buf, &pos, state.filter_buf[0..state.filter_len])?;
        iuAppendByte(out_buf, &pos, '"')?;
    }
    iuAppendByte(out_buf, &pos, '\n')?;

    // Column header row
    iuAppendBytes(out_buf, &pos, "      Package")?;
    iuAppendSpaces(out_buf, &pos, w_name - 7 + 2)?;
    iuAppendBytes(out_buf, &pos, "Current")?;
    iuAppendSpaces(out_buf, &pos, w_cur - 7 + 2)?;
    iuAppendBytes(out_buf, &pos, "Target")?;
    iuAppendSpaces(out_buf, &pos, w_tgt - 6 + 2)?;
    iuAppendBytes(out_buf, &pos, "Latest")?;
    if (opts.show_workspace) {
        iuAppendSpaces(out_buf, &pos, w_lat - 6 + 2)?;
        iuAppendBytes(out_buf, &pos, "Workspace")?;
    }
    iuAppendByte(out_buf, &pos, '\n')?;

    if (state.visible_count == 0) {
        iuAppendBytes(out_buf, &pos, "  (no matching outdated packages)\n")?;
    } else {
        const end_row = if (state.viewport_start + state.viewport_height <= state.visible_count)
            state.viewport_start + state.viewport_height
        else
            state.visible_count;

        let last_group_set = false;
        let last_group = IuDepGroup.Dependencies;
        let row = state.viewport_start;
        while (row < end_row) {
            const e = state.entries[state.visible_indices[row]];
            if (!last_group_set || e.group != last_group) {
                if (opts.ansi_colors) iuAppendBytes(out_buf, &pos, IU_ANSI_CYAN)?;
                iuAppendBytes(out_buf, &pos, "  [")?;
                iuAppendBytes(out_buf, &pos, iuDepGroupLabel(e.group))?;
                iuAppendBytes(out_buf, &pos, "]")?;
                if (opts.ansi_colors) iuAppendBytes(out_buf, &pos, IU_ANSI_RESET)?;
                iuAppendByte(out_buf, &pos, '\n')?;
                last_group = e.group;
                last_group_set = true;
            }

            // Cursor + checkbox
            iuAppendBytes(out_buf, &pos, if (row == state.cursor) "> " else "  ")?;
            iuAppendBytes(out_buf, &pos, if (e.selected) "[x] " else "[ ] ")?;

            // Name
            iuAppendBytes(out_buf, &pos, e.name)?;
            iuAppendSpaces(out_buf, &pos, w_name - e.name.len + 2)?;

            // Current
            iuAppendBytes(out_buf, &pos, e.current_version)?;
            iuAppendSpaces(out_buf, &pos, w_cur - e.current_version.len + 2)?;

            // Target (with diff highlight)
            let vbuf: [96]u8 = [0; 96];
            const t_written = formatVersionDiffHighlight(e.current_version, e.target_version, opts.ansi_colors, vbuf[0..])?;
            iuAppendBytes(out_buf, &pos, vbuf[0..t_written])?;
            iuAppendSpaces(out_buf, &pos, w_tgt - e.target_version.len + 2)?;

            // Latest (with diff highlight + `*` indicator if `use_latest`)
            const l_written = formatVersionDiffHighlight(e.current_version, e.latest_version, opts.ansi_colors, vbuf[0..])?;
            iuAppendBytes(out_buf, &pos, vbuf[0..l_written])?;
            if (e.use_latest) {
                iuAppendByte(out_buf, &pos, '*')?;
            }

            if (opts.show_workspace) {
                const pad = if (e.use_latest) (w_lat - e.latest_version.len + 1) else (w_lat - e.latest_version.len + 2);
                iuAppendSpaces(out_buf, &pos, pad)?;
                iuAppendBytes(out_buf, &pos, e.workspace)?;
            }
            iuAppendByte(out_buf, &pos, '\n')?;
            row += 1;
        }
    }

    if (opts.show_footer_help) {
        iuAppendBytes(out_buf, &pos, "Space: toggle | a: all | n: none | l: latest | Enter: confirm | Esc: cancel\n")?;
    }

    return pos;
}

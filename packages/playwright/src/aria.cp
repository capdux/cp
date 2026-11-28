// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/ariaSnapshot.ts | Synced: 2026-11-28
// ============================================================================
// packages/playwright/src/aria.cp — AriaSnapshot & Accessibility Tree Model
// Reuses cap:cdp AccessibilityDomain (AXNodeSummary) for zero-copy parsing.
// ============================================================================

const std = @import("std");
import { AccessibilityDomain, AXNodeSummary } from "cap:cdp";
import { PlaywrightError } from "./network";

pub struct AriaRoleNode {
    pub role: []const u8 = "",
    pub name: []const u8 = "",
    pub value: []const u8 = "",
    pub level: i32 = 0,
    pub checked: bool = false,
    pub disabled: bool = false,
    pub backend_dom_node_id: i64 = 0,
}

pub struct AriaSnapshot {
    allocator: std.mem.Allocator = undefined,
    pub nodes: std.ArrayList(AriaRoleNode) = .empty,
    owned_strings: std.ArrayList([]u8) = .empty,

    pub static fn init(alloc: std.mem.Allocator): AriaSnapshot {
        return AriaSnapshot{
            allocator: alloc,
            nodes: .empty,
            owned_strings: .empty,
        };
    }

    pub fn deinit(*_) {
        _.nodes.deinit(_.allocator);
        _.nodes = .empty;
        for (s of _.owned_strings.items) {
            _.allocator.free(s);
        }
        _.owned_strings.deinit(_.allocator);
        _.owned_strings = .empty;
    }

    ~init() {
        _.deinit();
    }

    fn intern(*_, s: []const u8): ([]const u8)!PlaywrightError {
        if (s.len == 0) return "";
        const duped = _.allocator.dupe(u8, s) catch return PlaywrightError.OutOfMemory;
        _.owned_strings.append(_.allocator, duped) catch {
            _.allocator.free(duped);
            return PlaywrightError.OutOfMemory;
        };
        return duped;
    }

    /// Parse a single CDP AXNode JSON object via cap:cdp AccessibilityDomain and append if non-ignored.
    pub fn addFromCdpAxNodeJson(*_, node_json: []const u8): void!PlaywrightError {
        const summary: AXNodeSummary = AccessibilityDomain.parseAXNodeSummary(node_json);
        if (summary.ignored) return;
        if (summary.role.len == 0) return;

        const r = _.intern(summary.role)?;
        const n = _.intern(summary.name)?;
        const v = _.intern(summary.value)?;

        _.nodes.append(_.allocator, AriaRoleNode{
            role: r,
            name: n,
            value: v,
            level: 0,
            checked: false,
            disabled: false,
            backend_dom_node_id: summary.backend_dom_node_id,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn addNode(*_, node: AriaRoleNode): void!PlaywrightError {
        const r = _.intern(node.role)?;
        const n = _.intern(node.name)?;
        const v = _.intern(node.value)?;
        _.nodes.append(_.allocator, AriaRoleNode{
            role: r,
            name: n,
            value: v,
            level: node.level,
            checked: node.checked,
            disabled: node.disabled,
            backend_dom_node_id: node.backend_dom_node_id,
        }) catch return PlaywrightError.OutOfMemory;
    }

    pub fn count(): usize {
        return _.nodes.items.len;
    }

    pub fn findByRoleAndName(role: []const u8, name: []const u8): AriaRoleNode? {
        for (node of _.nodes.items) {
            if (!std.mem.eql(u8, node.role, role)) continue;
            if (name.len == 0 || std.mem.eql(u8, node.name, name)) {
                return node;
            }
        }
        return null;
    }

    /// Render Playwright-compatible YAML-like aria snapshot string, e.g.:
    /// - heading "Welcome"
    /// - button "Submit"
    pub fn toYamlString(alloc: std.mem.Allocator): ([]u8)!PlaywrightError {
        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        for (node of _.nodes.items) {
            let indent: i32 = 0;
            while (indent < node.level) {
                buf.appendSlice(alloc, "  ") catch return PlaywrightError.OutOfMemory;
                indent += 1;
            }
            buf.appendSlice(alloc, "- ") catch return PlaywrightError.OutOfMemory;
            buf.appendSlice(alloc, node.role) catch return PlaywrightError.OutOfMemory;
            if (node.name.len > 0) {
                buf.appendSlice(alloc, " \"") catch return PlaywrightError.OutOfMemory;
                buf.appendSlice(alloc, node.name) catch return PlaywrightError.OutOfMemory;
                buf.append(alloc, '"') catch return PlaywrightError.OutOfMemory;
            }
            if (node.value.len > 0) {
                buf.appendSlice(alloc, ": ") catch return PlaywrightError.OutOfMemory;
                buf.appendSlice(alloc, node.value) catch return PlaywrightError.OutOfMemory;
            }
            buf.append(alloc, '\n') catch return PlaywrightError.OutOfMemory;
        }

        return buf.toOwnedSlice(alloc) catch return PlaywrightError.OutOfMemory;
    }
}

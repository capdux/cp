// Source: cp/lib/mermaid/diagram.cp (Declarative Mermaid Diagram Builders & Syntax-Safe Escaper)
// Pure CP zero-C-dependency Mermaid diagram generator supporting Flowchart,
// SequenceDiagram, ErDiagram, and StateDiagram with automatic label quoting/escaping.

const std = @import("std");

/// Error set for Mermaid diagram construction and rendering.
pub err MermaidError {
    InvalidIdentifier,
    DuplicateId,
    EmptyDiagram,
    UnknownNode,
    UnbalancedBlock,
    OutOfMemory,
}

/// Flowchart layout direction.
pub enum FlowDirection {
    TD,
    LR,
    BT,
    RL,

    pub fn asStr(): string {
        return switch (_) {
            .TD => "TD",
            .LR => "LR",
            .BT => "BT",
            .RL => "RL",
        };
    }
}

/// Flowchart node visual shape.
pub enum NodeShape {
    Rect,
    Rounded,
    Diamond,
    Cylinder,
    Circle,
    Hexagon,
}

/// Flowchart edge connector style.
pub enum EdgeStyle {
    SolidArrow,
    DottedArrow,
    ThickArrow,
    SolidLine,

    pub fn asStr(): string {
        return switch (_) {
            .SolidArrow => "-->",
            .DottedArrow => "-.->",
            .ThickArrow => "==>",
            .SolidLine => "---",
        };
    }
}

/// Sequence diagram message arrow style.
pub enum MsgArrow {
    Sync,
    Async,
    Reply,
    Solid,

    pub fn asStr(): string {
        return switch (_) {
            .Sync => "->>",
            .Async => "-->>",
            .Reply => "-->",
            .Solid => "->",
        };
    }
}

/// Entity-Relationship cardinality on one side of a relation.
pub enum ErCardinality {
    OneToOne,
    OneToMany,
    ZeroOrOne,
    ZeroOrMany,

    pub fn leftStr(): string {
        return switch (_) {
            .OneToOne => "||",
            .OneToMany => "}|",
            .ZeroOrOne => "|o",
            .ZeroOrMany => "}o",
        };
    }

    pub fn rightStr(): string {
        return switch (_) {
            .OneToOne => "||",
            .OneToMany => "|{",
            .ZeroOrOne => "o|",
            .ZeroOrMany => "o{",
        };
    }
}

fn appendSlice(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! MermaidError {
    buf.appendSlice(alloc, s) catch return MermaidError.OutOfMemory;
}

fn appendByte(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), b: u8): void ! MermaidError {
    buf.append(alloc, b) catch return MermaidError.OutOfMemory;
}

fn appendIndent(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), depth: usize): void ! MermaidError {
    let i: usize = 0;
    while (i < depth) : (i += 1) {
        appendSlice(alloc, buf, "    ")?;
    }
}

fn dupeStr(alloc: std.mem.Allocator, s: string): ([]u8) ! MermaidError {
    return alloc.dupe(u8, s) catch return MermaidError.OutOfMemory;
}

/// Validates a Mermaid identifier (`[A-Za-z0-9_-]+`, non-empty).
pub fn isValidIdentifier(id: string): bool {
    if (id.len == 0) {
        return false;
    }
    for (c of id) {
        const is_alpha = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        const is_digit = (c >= '0' && c <= '9');
        const is_punct = (c == '_' || c == '-');
        if (!is_alpha && !is_digit && !is_punct) {
            return false;
        }
    }
    return true;
}

fn isValidStateId(id: string): bool {
    if (std.mem.eql(u8, id, "[*]")) {
        return true;
    }
    return isValidIdentifier(id);
}

fn needsQuoting(label: string): bool {
    if (label.len == 0) {
        return true;
    }
    if (label[0] == ' ' || label[label.len - 1] == ' ') {
        return true;
    }
    for (c of label) {
        if (c == '(' || c == ')' || c == '[' || c == ']' || c == '{' || c == '}' ||
            c == '"' || c == '\'' || c == '<' || c == '>' || c == '&' || c == '#' ||
            c == ';' || c == '|' || c == ':' || c == '/' || c == '\\' ||
            c == '\n' || c == '\r' || c == '\t') {
            return true;
        }
    }
    return false;
}

/// Appends escaped label content suitable for inside `"..."` in Mermaid syntax.
fn appendEscapedLabelBody(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! MermaidError {
    for (c of s) {
        switch (c) {
            '"' => appendSlice(alloc, buf, "#quot;")?,
            '<' => appendSlice(alloc, buf, "&lt;")?,
            '>' => appendSlice(alloc, buf, "&gt;")?,
            '\n' => appendSlice(alloc, buf, "<br/>")?,
            '\r' => {},
            '\t' => appendByte(alloc, buf, ' ')?,
            else => appendByte(alloc, buf, c)?,
        }
    }
}

/// Appends a label, automatically wrapping in `"..."` and escaping special characters if needed.
fn appendSafeLabel(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! MermaidError {
    if (needsQuoting(s)) {
        appendByte(alloc, buf, '"')?;
        appendEscapedLabelBody(alloc, buf, s)?;
        appendByte(alloc, buf, '"')?;
    } else {
        appendSlice(alloc, buf, s)?;
    }
}

/// Appends a single-line text value (e.g. sequence message or note), escaping newlines and HTML angle brackets.
fn appendSafeLineText(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! MermaidError {
    for (c of s) {
        switch (c) {
            '<' => appendSlice(alloc, buf, "&lt;")?,
            '>' => appendSlice(alloc, buf, "&gt;")?,
            '\n' => appendSlice(alloc, buf, "<br/>")?,
            '\r' => {},
            ';' => appendSlice(alloc, buf, "#59;")?,
            else => appendByte(alloc, buf, c)?,
        }
    }
}

fn wrapMarkdownFence(alloc: std.mem.Allocator, raw: []u8): ([]u8) ! MermaidError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);
    appendSlice(alloc, *buf, "```mermaid\n")?;
    appendSlice(alloc, *buf, raw)?;
    if (raw.len == 0 || raw[raw.len - 1] != '\n') {
        appendByte(alloc, *buf, '\n')?;
    }
    appendSlice(alloc, *buf, "```\n")?;
    return buf.toOwnedSlice(alloc) catch return MermaidError.OutOfMemory;
}

// ============================================================================
// 1. FlowchartBuilder
// ============================================================================

enum FlowItemKind {
    Node,
    Edge,
    BeginSubgraph,
    EndSubgraph,
}

struct FlowItem {
    kind: FlowItemKind;
    id_or_from: []u8;
    to_id: []u8;
    label: []u8;
    shape: NodeShape;
    edge_style: EdgeStyle;
}

/// Declarative builder for Mermaid `flowchart` diagrams.
pub struct FlowchartBuilder {
    alloc: std.mem.Allocator;
    direction: FlowDirection;
    items: std.ArrayList(FlowItem) = .empty;
    node_ids: std.ArrayList([]u8) = .empty;
    subgraph_depth: usize = 0;
    strict_nodes: bool = false;

    /// Initializes a new `FlowchartBuilder` with the given layout direction.
    pub static fn init(alloc: std.mem.Allocator, direction: FlowDirection): FlowchartBuilder {
        return FlowchartBuilder{
            alloc: alloc,
            direction: direction,
            items: .empty,
            node_ids: .empty,
            subgraph_depth: 0,
            strict_nodes: false,
        };
    }

    /// Releases all owned strings and internal buffers.
    pub fn deinit(*_) {
        for (item of _.items.items) {
            _.alloc.free(item.id_or_from);
            _.alloc.free(item.to_id);
            _.alloc.free(item.label);
        }
        _.items.deinit(_.alloc);
        for (nid of _.node_ids.items) {
            _.alloc.free(nid);
        }
        _.node_ids.deinit(_.alloc);
    }

    /// Enables or disables strict node existence checking in `addEdge`.
    pub fn setStrict(*_, strict: bool) {
        _.strict_nodes = strict;
    }

    fn hasNodeId(id: string): bool {
        for (existing of _.node_ids.items) {
            if (std.mem.eql(u8, existing, id)) {
                return true;
            }
        }
        return false;
    }

    fn registerNodeId(*_, id: string): void ! MermaidError {
        const dup = dupeStr(_.alloc, id)?;
        errdefer _.alloc.free(dup);
        _.node_ids.append(_.alloc, dup) catch return MermaidError.OutOfMemory;
    }

    /// Adds a node with a validated identifier, label, and shape.
    pub fn addNode(*_, id: string, label: string, shape: NodeShape): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        if (_.hasNodeId(id)) {
            return MermaidError.DuplicateId;
        }

        const owned_id = dupeStr(_.alloc, id)?;
        errdefer _.alloc.free(owned_id);
        const owned_empty = dupeStr(_.alloc, "")?;
        errdefer _.alloc.free(owned_empty);
        const owned_label = dupeStr(_.alloc, label)?;
        errdefer _.alloc.free(owned_label);

        _.registerNodeId(id)?;
        _.items.append(_.alloc, FlowItem{
            kind: .Node,
            id_or_from: owned_id,
            to_id: owned_empty,
            label: owned_label,
            shape: shape,
            edge_style: .SolidArrow,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Adds a directed or undirected edge between two nodes with an optional label.
    pub fn addEdge(*_, from_id: string, to_id: string, label: string, style: EdgeStyle): void ! MermaidError {
        if (!isValidIdentifier(from_id) || !isValidIdentifier(to_id)) {
            return MermaidError.InvalidIdentifier;
        }
        if (_.strict_nodes && (!_.hasNodeId(from_id) || !_.hasNodeId(to_id))) {
            return MermaidError.UnknownNode;
        }

        const owned_from = dupeStr(_.alloc, from_id)?;
        errdefer _.alloc.free(owned_from);
        const owned_to = dupeStr(_.alloc, to_id)?;
        errdefer _.alloc.free(owned_to);
        const owned_label = dupeStr(_.alloc, label)?;
        errdefer _.alloc.free(owned_label);

        _.items.append(_.alloc, FlowItem{
            kind: .Edge,
            id_or_from: owned_from,
            to_id: owned_to,
            label: owned_label,
            shape: .Rect,
            edge_style: style,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Opens a nested `subgraph` block with the given ID and title.
    pub fn beginSubgraph(*_, id: string, title: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        const owned_id = dupeStr(_.alloc, id)?;
        errdefer _.alloc.free(owned_id);
        const owned_empty = dupeStr(_.alloc, "")?;
        errdefer _.alloc.free(owned_empty);
        const owned_title = dupeStr(_.alloc, title)?;
        errdefer _.alloc.free(owned_title);

        _.items.append(_.alloc, FlowItem{
            kind: .BeginSubgraph,
            id_or_from: owned_id,
            to_id: owned_empty,
            label: owned_title,
            shape: .Rect,
            edge_style: .SolidArrow,
        }) catch return MermaidError.OutOfMemory;
        _.subgraph_depth += 1;
    }

    /// Closes the most recently opened `subgraph` block.
    pub fn endSubgraph(*_): void ! MermaidError {
        if (_.subgraph_depth == 0) {
            return MermaidError.UnbalancedBlock;
        }
        const owned_a = dupeStr(_.alloc, "")?;
        errdefer _.alloc.free(owned_a);
        const owned_b = dupeStr(_.alloc, "")?;
        errdefer _.alloc.free(owned_b);
        const owned_c = dupeStr(_.alloc, "")?;
        errdefer _.alloc.free(owned_c);

        _.items.append(_.alloc, FlowItem{
            kind: .EndSubgraph,
            id_or_from: owned_a,
            to_id: owned_b,
            label: owned_c,
            shape: .Rect,
            edge_style: .SolidArrow,
        }) catch return MermaidError.OutOfMemory;
        _.subgraph_depth -= 1;
    }

    /// Renders the flowchart into a newly allocated Mermaid text buffer.
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        if (_.items.items.len == 0) {
            return MermaidError.EmptyDiagram;
        }
        if (_.subgraph_depth != 0) {
            return MermaidError.UnbalancedBlock;
        }

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        appendSlice(alloc, *buf, "flowchart ")?;
        appendSlice(alloc, *buf, _.direction.asStr())?;
        appendByte(alloc, *buf, '\n')?;

        let indent: usize = 1;
        for (item of _.items.items) {
            switch (item.kind) {
                .Node => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, item.id_or_from)?;
                    switch (item.shape) {
                        .Rect => {
                            appendByte(alloc, *buf, '[')?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendByte(alloc, *buf, ']')?;
                        },
                        .Rounded => {
                            appendByte(alloc, *buf, '(')?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendByte(alloc, *buf, ')')?;
                        },
                        .Diamond => {
                            appendByte(alloc, *buf, '{')?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendByte(alloc, *buf, '}')?;
                        },
                        .Cylinder => {
                            appendSlice(alloc, *buf, "[(")?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendSlice(alloc, *buf, ")]")?;
                        },
                        .Circle => {
                            appendSlice(alloc, *buf, "((")?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendSlice(alloc, *buf, "))")?;
                        },
                        .Hexagon => {
                            appendSlice(alloc, *buf, "{{")?;
                            appendSafeLabel(alloc, *buf, item.label)?;
                            appendSlice(alloc, *buf, "}}")?;
                        },
                    }
                    appendByte(alloc, *buf, '\n')?;
                },
                .Edge => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, item.id_or_from)?;
                    appendByte(alloc, *buf, ' ')?;
                    appendSlice(alloc, *buf, item.edge_style.asStr())?;
                    if (item.label.len > 0) {
                        appendByte(alloc, *buf, '|')?;
                        appendSafeLabel(alloc, *buf, item.label)?;
                        appendByte(alloc, *buf, '|')?;
                    }
                    appendByte(alloc, *buf, ' ')?;
                    appendSlice(alloc, *buf, item.to_id)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .BeginSubgraph => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "subgraph ")?;
                    appendSlice(alloc, *buf, item.id_or_from)?;
                    if (item.label.len > 0) {
                        appendSlice(alloc, *buf, " [")?;
                        appendSafeLabel(alloc, *buf, item.label)?;
                        appendByte(alloc, *buf, ']')?;
                    }
                    appendByte(alloc, *buf, '\n')?;
                    indent += 1;
                },
                .EndSubgraph => {
                    if (indent > 1) {
                        indent -= 1;
                    }
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "end\n")?;
                },
            }
        }

        return buf.toOwnedSlice(alloc) catch return MermaidError.OutOfMemory;
    }

    /// Renders the flowchart wrapped in a Markdown ```` ```mermaid ```` code fence.
    pub fn renderMarkdownBlock(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        const raw = _.render(alloc)?;
        defer alloc.free(raw);
        return wrapMarkdownFence(alloc, raw);
    }
}

// ============================================================================
// 2. SequenceBuilder
// ============================================================================

enum SeqItemKind {
    Participant,
    Actor,
    Message,
    NoteOver,
    Activate,
    Deactivate,
    BeginLoop,
    BeginAlt,
    ElseBranch,
    EndBlock,
}

struct SeqItem {
    kind: SeqItemKind;
    a: []u8;
    b: []u8;
    text: []u8;
    arrow: MsgArrow;
}

/// Declarative builder for Mermaid `sequenceDiagram` diagrams.
pub struct SequenceBuilder {
    alloc: std.mem.Allocator;
    items: std.ArrayList(SeqItem) = .empty;
    participant_ids: std.ArrayList([]u8) = .empty;
    block_depth: usize = 0;

    /// Initializes a new `SequenceBuilder`.
    pub static fn init(alloc: std.mem.Allocator): SequenceBuilder {
        return SequenceBuilder{
            alloc: alloc,
            items: .empty,
            participant_ids: .empty,
            block_depth: 0,
        };
    }

    /// Releases all owned strings and internal buffers.
    pub fn deinit(*_) {
        for (item of _.items.items) {
            _.alloc.free(item.a);
            _.alloc.free(item.b);
            _.alloc.free(item.text);
        }
        _.items.deinit(_.alloc);
        for (pid of _.participant_ids.items) {
            _.alloc.free(pid);
        }
        _.participant_ids.deinit(_.alloc);
    }

    fn hasParticipant(id: string): bool {
        for (existing of _.participant_ids.items) {
            if (std.mem.eql(u8, existing, id)) {
                return true;
            }
        }
        return false;
    }

    fn appendSeqItem(*_, kind: SeqItemKind, a: string, b: string, text: string, arrow: MsgArrow): void ! MermaidError {
        const oa = dupeStr(_.alloc, a)?;
        errdefer _.alloc.free(oa);
        const ob = dupeStr(_.alloc, b)?;
        errdefer _.alloc.free(ob);
        const ot = dupeStr(_.alloc, text)?;
        errdefer _.alloc.free(ot);

        _.items.append(_.alloc, SeqItem{
            kind: kind,
            a: oa,
            b: ob,
            text: ot,
            arrow: arrow,
        }) catch return MermaidError.OutOfMemory;
    }

    fn addRole(*_, kind: SeqItemKind, id: string, alias: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        if (_.hasParticipant(id)) {
            return MermaidError.DuplicateId;
        }
        const dup = dupeStr(_.alloc, id)?;
        errdefer _.alloc.free(dup);
        _.participant_ids.append(_.alloc, dup) catch return MermaidError.OutOfMemory;
        _.appendSeqItem(kind, id, "", alias, .Sync)?;
    }

    /// Declares a `participant` with an optional display alias.
    pub fn addParticipant(*_, id: string, alias: string): void ! MermaidError {
        _.addRole(.Participant, id, alias)?;
    }

    /// Declares an `actor` with an optional display alias.
    pub fn addActor(*_, id: string, alias: string): void ! MermaidError {
        _.addRole(.Actor, id, alias)?;
    }

    /// Appends a message arrow from `from_id` to `to_id` with label `text`.
    pub fn message(*_, from_id: string, to_id: string, text: string, arrow: MsgArrow): void ! MermaidError {
        if (!isValidIdentifier(from_id) || !isValidIdentifier(to_id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendSeqItem(.Message, from_id, to_id, text, arrow)?;
    }

    /// Appends a `Note over <participant>: <text>` annotation.
    pub fn noteOver(*_, participant: string, text: string): void ! MermaidError {
        if (!isValidIdentifier(participant)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendSeqItem(.NoteOver, participant, "", text, .Sync)?;
    }

    /// Activates the lifeline bar for `id`.
    pub fn activate(*_, id: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendSeqItem(.Activate, id, "", "", .Sync)?;
    }

    /// Deactivates the lifeline bar for `id`.
    pub fn deactivate(*_, id: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendSeqItem(.Deactivate, id, "", "", .Sync)?;
    }

    /// Opens a `loop <label>` block.
    pub fn beginLoop(*_, label: string): void ! MermaidError {
        _.appendSeqItem(.BeginLoop, "", "", label, .Sync)?;
        _.block_depth += 1;
    }

    /// Opens an `alt <cond>` conditional block.
    pub fn beginAlt(*_, cond: string): void ! MermaidError {
        _.appendSeqItem(.BeginAlt, "", "", cond, .Sync)?;
        _.block_depth += 1;
    }

    /// Emits an `else <cond>` branch inside an open `alt` block.
    pub fn elseBranch(*_, cond: string): void ! MermaidError {
        if (_.block_depth == 0) {
            return MermaidError.UnbalancedBlock;
        }
        _.appendSeqItem(.ElseBranch, "", "", cond, .Sync)?;
    }

    /// Closes the most recently opened `loop` or `alt` block with `end`.
    pub fn endBlock(*_): void ! MermaidError {
        if (_.block_depth == 0) {
            return MermaidError.UnbalancedBlock;
        }
        _.appendSeqItem(.EndBlock, "", "", "", .Sync)?;
        _.block_depth -= 1;
    }

    /// Renders the sequence diagram into a newly allocated Mermaid text buffer.
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        if (_.items.items.len == 0) {
            return MermaidError.EmptyDiagram;
        }
        if (_.block_depth != 0) {
            return MermaidError.UnbalancedBlock;
        }

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        appendSlice(alloc, *buf, "sequenceDiagram\n")?;

        let indent: usize = 1;
        for (item of _.items.items) {
            switch (item.kind) {
                .Participant => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "participant ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    if (item.text.len > 0) {
                        appendSlice(alloc, *buf, " as ")?;
                        appendSafeLineText(alloc, *buf, item.text)?;
                    }
                    appendByte(alloc, *buf, '\n')?;
                },
                .Actor => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "actor ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    if (item.text.len > 0) {
                        appendSlice(alloc, *buf, " as ")?;
                        appendSafeLineText(alloc, *buf, item.text)?;
                    }
                    appendByte(alloc, *buf, '\n')?;
                },
                .Message => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendSlice(alloc, *buf, item.arrow.asStr())?;
                    appendSlice(alloc, *buf, item.b)?;
                    appendSlice(alloc, *buf, ": ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .NoteOver => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "Note over ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendSlice(alloc, *buf, ": ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .Activate => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "activate ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .Deactivate => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "deactivate ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .BeginLoop => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "loop ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                    indent += 1;
                },
                .BeginAlt => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "alt ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                    indent += 1;
                },
                .ElseBranch => {
                    const else_indent = if (indent > 1) indent - 1 else 1;
                    appendIndent(alloc, *buf, else_indent)?;
                    appendSlice(alloc, *buf, "else ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .EndBlock => {
                    if (indent > 1) {
                        indent -= 1;
                    }
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "end\n")?;
                },
            }
        }

        return buf.toOwnedSlice(alloc) catch return MermaidError.OutOfMemory;
    }

    /// Renders the sequence diagram wrapped in a Markdown ```` ```mermaid ```` code fence.
    pub fn renderMarkdownBlock(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        const raw = _.render(alloc)?;
        defer alloc.free(raw);
        return wrapMarkdownFence(alloc, raw);
    }
}

// ============================================================================
// 3. ErBuilder
// ============================================================================

struct ErAttr {
    type_name: []u8;
    attr_name: []u8;
    key_tag: []u8;
}

struct ErEntity {
    name: []u8;
    attrs: std.ArrayList(ErAttr) = .empty;
}

struct ErRelation {
    left: []u8;
    card_left: ErCardinality;
    right: []u8;
    card_right: ErCardinality;
    label: []u8;
}

/// Declarative builder for Mermaid `erDiagram` schemas.
pub struct ErBuilder {
    alloc: std.mem.Allocator;
    entities: std.ArrayList(ErEntity) = .empty;
    relations: std.ArrayList(ErRelation) = .empty;

    /// Initializes a new `ErBuilder`.
    pub static fn init(alloc: std.mem.Allocator): ErBuilder {
        return ErBuilder{
            alloc: alloc,
            entities: .empty,
            relations: .empty,
        };
    }

    /// Releases all owned entity definitions, attributes, and relations.
    pub fn deinit(*_) {
        for (ent of _.entities.items) {
            _.alloc.free(ent.name);
            for (attr of ent.attrs.items) {
                _.alloc.free(attr.type_name);
                _.alloc.free(attr.attr_name);
                _.alloc.free(attr.key_tag);
            }
            let mut_attrs = ent.attrs;
            mut_attrs.deinit(_.alloc);
        }
        _.entities.deinit(_.alloc);

        for (rel of _.relations.items) {
            _.alloc.free(rel.left);
            _.alloc.free(rel.right);
            _.alloc.free(rel.label);
        }
        _.relations.deinit(_.alloc);
    }

    fn findEntityIndex(name: string): usize? {
        let i: usize = 0;
        while (i < _.entities.items.len) : (i += 1) {
            if (std.mem.eql(u8, _.entities.items[i].name, name)) {
                return i;
            }
        }
        return null;
    }

    /// Registers a new entity in the ER diagram.
    pub fn addEntity(*_, name: string): void ! MermaidError {
        if (!isValidIdentifier(name)) {
            return MermaidError.InvalidIdentifier;
        }
        if (_.findEntityIndex(name) != null) {
            return MermaidError.DuplicateId;
        }
        const owned_name = dupeStr(_.alloc, name)?;
        errdefer _.alloc.free(owned_name);

        _.entities.append(_.alloc, ErEntity{
            name: owned_name,
            attrs: .empty,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Adds a typed attribute (with optional `PK`, `FK`, or `UK` key tag) to an existing entity.
    pub fn addAttribute(*_, entity: string, type_name: string, attr_name: string, key_tag: string): void ! MermaidError {
        if (!isValidIdentifier(entity) || !isValidIdentifier(type_name) || !isValidIdentifier(attr_name)) {
            return MermaidError.InvalidIdentifier;
        }
        const idx = _.findEntityIndex(entity) orelse return MermaidError.UnknownNode;

        const ot = dupeStr(_.alloc, type_name)?;
        errdefer _.alloc.free(ot);
        const oa = dupeStr(_.alloc, attr_name)?;
        errdefer _.alloc.free(oa);
        const ok = dupeStr(_.alloc, key_tag)?;
        errdefer _.alloc.free(ok);

        _.entities.items[idx].attrs.append(_.alloc, ErAttr{
            type_name: ot,
            attr_name: oa,
            key_tag: ok,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Adds a cardinality relationship between `left` and `right` entities.
    pub fn addRelation(*_, left: string, card_left: ErCardinality, right: string, card_right: ErCardinality, label: string): void ! MermaidError {
        if (!isValidIdentifier(left) || !isValidIdentifier(right)) {
            return MermaidError.InvalidIdentifier;
        }
        const ol = dupeStr(_.alloc, left)?;
        errdefer _.alloc.free(ol);
        const or_id = dupeStr(_.alloc, right)?;
        errdefer _.alloc.free(or_id);
        const olabel = dupeStr(_.alloc, label)?;
        errdefer _.alloc.free(olabel);

        _.relations.append(_.alloc, ErRelation{
            left: ol,
            card_left: card_left,
            right: or_id,
            card_right: card_right,
            label: olabel,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Renders the ER diagram into a newly allocated Mermaid text buffer.
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        if (_.entities.items.len == 0 && _.relations.items.len == 0) {
            return MermaidError.EmptyDiagram;
        }

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        appendSlice(alloc, *buf, "erDiagram\n")?;

        for (ent of _.entities.items) {
            appendIndent(alloc, *buf, 1)?;
            appendSlice(alloc, *buf, ent.name)?;
            appendSlice(alloc, *buf, " {\n")?;
            for (attr of ent.attrs.items) {
                appendIndent(alloc, *buf, 2)?;
                appendSlice(alloc, *buf, attr.type_name)?;
                appendByte(alloc, *buf, ' ')?;
                appendSlice(alloc, *buf, attr.attr_name)?;
                if (attr.key_tag.len > 0) {
                    appendByte(alloc, *buf, ' ')?;
                    appendSlice(alloc, *buf, attr.key_tag)?;
                }
                appendByte(alloc, *buf, '\n')?;
            }
            appendIndent(alloc, *buf, 1)?;
            appendSlice(alloc, *buf, "}\n")?;
        }

        for (rel of _.relations.items) {
            appendIndent(alloc, *buf, 1)?;
            appendSlice(alloc, *buf, rel.left)?;
            appendByte(alloc, *buf, ' ')?;
            appendSlice(alloc, *buf, rel.card_left.leftStr())?;
            appendSlice(alloc, *buf, "--")?;
            appendSlice(alloc, *buf, rel.card_right.rightStr())?;
            appendByte(alloc, *buf, ' ')?;
            appendSlice(alloc, *buf, rel.right)?;
            appendSlice(alloc, *buf, " : ")?;
            appendSafeLabel(alloc, *buf, rel.label)?;
            appendByte(alloc, *buf, '\n')?;
        }

        return buf.toOwnedSlice(alloc) catch return MermaidError.OutOfMemory;
    }

    /// Renders the ER diagram wrapped in a Markdown ```` ```mermaid ```` code fence.
    pub fn renderMarkdownBlock(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        const raw = _.render(alloc)?;
        defer alloc.free(raw);
        return wrapMarkdownFence(alloc, raw);
    }
}

// ============================================================================
// 4. StateBuilder
// ============================================================================

enum StateItemKind {
    StateDecl,
    Transition,
    BeginComposite,
    EndComposite,
    NoteRight,
}

struct StateItem {
    kind: StateItemKind;
    a: []u8;
    b: []u8;
    text: []u8;
}

/// Declarative builder for Mermaid `stateDiagram-v2` state machines.
pub struct StateBuilder {
    alloc: std.mem.Allocator;
    items: std.ArrayList(StateItem) = .empty;
    state_ids: std.ArrayList([]u8) = .empty;
    composite_depth: usize = 0;

    /// Initializes a new `StateBuilder`.
    pub static fn init(alloc: std.mem.Allocator): StateBuilder {
        return StateBuilder{
            alloc: alloc,
            items: .empty,
            state_ids: .empty,
            composite_depth: 0,
        };
    }

    /// Releases all owned state declarations and transitions.
    pub fn deinit(*_) {
        for (item of _.items.items) {
            _.alloc.free(item.a);
            _.alloc.free(item.b);
            _.alloc.free(item.text);
        }
        _.items.deinit(_.alloc);
        for (sid of _.state_ids.items) {
            _.alloc.free(sid);
        }
        _.state_ids.deinit(_.alloc);
    }

    fn hasStateId(id: string): bool {
        for (existing of _.state_ids.items) {
            if (std.mem.eql(u8, existing, id)) {
                return true;
            }
        }
        return false;
    }

    fn appendStateItem(*_, kind: StateItemKind, a: string, b: string, text: string): void ! MermaidError {
        const oa = dupeStr(_.alloc, a)?;
        errdefer _.alloc.free(oa);
        const ob = dupeStr(_.alloc, b)?;
        errdefer _.alloc.free(ob);
        const ot = dupeStr(_.alloc, text)?;
        errdefer _.alloc.free(ot);

        _.items.append(_.alloc, StateItem{
            kind: kind,
            a: oa,
            b: ob,
            text: ot,
        }) catch return MermaidError.OutOfMemory;
    }

    /// Declares a named state with a human-readable description.
    pub fn addState(*_, id: string, description: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        if (_.hasStateId(id)) {
            return MermaidError.DuplicateId;
        }
        const dup = dupeStr(_.alloc, id)?;
        errdefer _.alloc.free(dup);
        _.state_ids.append(_.alloc, dup) catch return MermaidError.OutOfMemory;
        _.appendStateItem(.StateDecl, id, "", description)?;
    }

    /// Adds a state transition `from_id --> to_id` (supports `"[*]"` start/end pseudo-state).
    pub fn addTransition(*_, from_id: string, to_id: string, label: string): void ! MermaidError {
        if (!isValidStateId(from_id) || !isValidStateId(to_id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendStateItem(.Transition, from_id, to_id, label)?;
    }

    /// Opens a composite state block `state id { ... }`.
    pub fn beginComposite(*_, id: string, title: string): void ! MermaidError {
        if (!isValidIdentifier(id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendStateItem(.BeginComposite, id, "", title)?;
        _.composite_depth += 1;
    }

    /// Closes the most recently opened composite state block.
    pub fn endComposite(*_): void ! MermaidError {
        if (_.composite_depth == 0) {
            return MermaidError.UnbalancedBlock;
        }
        _.appendStateItem(.EndComposite, "", "", "")?;
        _.composite_depth -= 1;
    }

    /// Attaches a `note right of <state_id>` annotation.
    pub fn noteRightOf(*_, state_id: string, text: string): void ! MermaidError {
        if (!isValidIdentifier(state_id)) {
            return MermaidError.InvalidIdentifier;
        }
        _.appendStateItem(.NoteRight, state_id, "", text)?;
    }

    /// Renders the state diagram into a newly allocated Mermaid text buffer.
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        if (_.items.items.len == 0) {
            return MermaidError.EmptyDiagram;
        }
        if (_.composite_depth != 0) {
            return MermaidError.UnbalancedBlock;
        }

        let buf: std.ArrayList(u8) = .empty;
        errdefer buf.deinit(alloc);

        appendSlice(alloc, *buf, "stateDiagram-v2\n")?;

        let indent: usize = 1;
        for (item of _.items.items) {
            switch (item.kind) {
                .StateDecl => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "state ")?;
                    appendSafeLabel(alloc, *buf, item.text)?;
                    appendSlice(alloc, *buf, " as ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendByte(alloc, *buf, '\n')?;
                },
                .Transition => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendSlice(alloc, *buf, " --> ")?;
                    appendSlice(alloc, *buf, item.b)?;
                    if (item.text.len > 0) {
                        appendSlice(alloc, *buf, ": ")?;
                        appendSafeLineText(alloc, *buf, item.text)?;
                    }
                    appendByte(alloc, *buf, '\n')?;
                },
                .BeginComposite => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "state ")?;
                    if (item.text.len > 0) {
                        appendSafeLabel(alloc, *buf, item.text)?;
                        appendSlice(alloc, *buf, " as ")?;
                    }
                    appendSlice(alloc, *buf, item.a)?;
                    appendSlice(alloc, *buf, " {\n")?;
                    indent += 1;
                },
                .EndComposite => {
                    if (indent > 1) {
                        indent -= 1;
                    }
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "}\n")?;
                },
                .NoteRight => {
                    appendIndent(alloc, *buf, indent)?;
                    appendSlice(alloc, *buf, "note right of ")?;
                    appendSlice(alloc, *buf, item.a)?;
                    appendSlice(alloc, *buf, ": ")?;
                    appendSafeLineText(alloc, *buf, item.text)?;
                    appendByte(alloc, *buf, '\n')?;
                },
            }
        }

        return buf.toOwnedSlice(alloc) catch return MermaidError.OutOfMemory;
    }

    /// Renders the state diagram wrapped in a Markdown ```` ```mermaid ```` code fence.
    pub fn renderMarkdownBlock(alloc: std.mem.Allocator): ([]u8) ! MermaidError {
        const raw = _.render(alloc)?;
        defer alloc.free(raw);
        return wrapMarkdownFence(alloc, raw);
    }
}

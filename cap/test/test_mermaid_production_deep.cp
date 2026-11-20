// Source: cp/cap/test/test_mermaid_production_deep.cp
// Deep Production Test Suite for cap:mermaid & cap.mermaid
// Covers:
//   1. FlowchartBuilder: multi-shape nodes, nested subgraphs, edge styles, special char auto-quoting & escaping
//   2. SequenceBuilder: participants/actors, Sync/Async/Reply arrows, activate/deactivate, alt/else/loop/end indentation
//   3. ErBuilder: entity attributes (PK/FK/UK) and cardinality relations (||--o{, ||--|{)
//   4. StateBuilder + error guards (InvalidIdentifier, DuplicateId, UnknownNode, EmptyDiagram, UnbalancedBlock) & zero leaks
//   5. Dynamic namespace mounting via `import cap from "cap"; cap.mermaid`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    MermaidError,
    FlowDirection,
    NodeShape,
    EdgeStyle,
    MsgArrow,
    ErCardinality,
    isValidIdentifier,
    FlowchartBuilder,
    SequenceBuilder,
    ErBuilder,
    StateBuilder,
} from "cap:mermaid";
import cap from "cap";

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn testFlowchartAndEscaping()! {
    const alloc = std.testing.allocator;
    let fc = FlowchartBuilder.init(alloc, .TD);
    defer fc.deinit();

    fc.addNode("start", "Start", .Circle)?;
    fc.beginSubgraph("sg_core", "Core Pipeline (v2)")?;
    fc.addNode("ingest", "Ingest (Extra Info) [v2]", .Rect)?;
    fc.addNode("decide", "Is \"valid\" <ok>?\nCheck", .Diamond)?;
    fc.addNode("store", "Vector DB", .Cylinder)?;
    fc.addNode("round_n", "Normalize", .Rounded)?;
    fc.addNode("hex_n", "Dispatch", .Hexagon)?;
    fc.endSubgraph()?;

    fc.addEdge("start", "ingest", "", .SolidArrow)?;
    fc.addEdge("ingest", "decide", "step (1)", .ThickArrow)?;
    fc.addEdge("decide", "store", "yes", .DottedArrow)?;
    fc.addEdge("round_n", "hex_n", "", .SolidLine)?;

    const rendered = fc.render(alloc)?;
    defer alloc.free(rendered);

    expect_eq(containsSubstr(rendered, "flowchart TD\n"), true);
    expect_eq(containsSubstr(rendered, "start((Start))"), true);
    expect_eq(containsSubstr(rendered, "subgraph sg_core [\"Core Pipeline (v2)\"]"), true);
    expect_eq(containsSubstr(rendered, "ingest[\"Ingest (Extra Info) [v2]\"]"), true);
    expect_eq(containsSubstr(rendered, "decide{\"Is #quot;valid#quot; &lt;ok&gt;?<br/>Check\"}"), true);
    expect_eq(containsSubstr(rendered, "store[(Vector DB)]"), true);
    expect_eq(containsSubstr(rendered, "round_n(Normalize)"), true);
    expect_eq(containsSubstr(rendered, "hex_n{{Dispatch}}"), true);
    expect_eq(containsSubstr(rendered, "ingest ==>|\"step (1)\"| decide"), true);
    expect_eq(containsSubstr(rendered, "decide -.->|yes| store"), true);
    expect_eq(containsSubstr(rendered, "round_n --- hex_n"), true);

    const fenced = fc.renderMarkdownBlock(alloc)?;
    defer alloc.free(fenced);
    expect_eq(std.mem.startsWith(u8, fenced, "```mermaid\nflowchart TD\n"), true);
    expect_eq(std.mem.endsWith(u8, fenced, "```\n"), true);
}

fn testSequenceDiagramControlBlocks()! {
    const alloc = std.testing.allocator;
    let seq = SequenceBuilder.init(alloc);
    defer seq.deinit();

    seq.addActor("user", "Developer")?;
    seq.addParticipant("api", "Gateway (v2)")?;
    seq.addParticipant("worker", "Agent Worker")?;

    seq.message("user", "api", "POST /run", .Sync)?;
    seq.activate("api")?;
    seq.beginAlt("cache hit")?;
    seq.message("api", "user", "200 Cached", .Async)?;
    seq.elseBranch("cache miss")?;
    seq.beginLoop("retry 3x")?;
    seq.message("api", "worker", "dispatch task", .Sync)?;
    seq.message("worker", "api", "result", .Reply)?;
    seq.endBlock()?;
    seq.noteOver("worker", "Done\nLogged")?;
    seq.endBlock()?;
    seq.deactivate("api")?;

    const rendered = seq.render(alloc)?;
    defer alloc.free(rendered);

    expect_eq(containsSubstr(rendered, "sequenceDiagram\n"), true);
    expect_eq(containsSubstr(rendered, "    actor user as Developer\n"), true);
    expect_eq(containsSubstr(rendered, "    participant api as Gateway (v2)\n"), true);
    expect_eq(containsSubstr(rendered, "    user->>api: POST /run\n"), true);
    expect_eq(containsSubstr(rendered, "    activate api\n"), true);
    expect_eq(containsSubstr(rendered, "    alt cache hit\n"), true);
    expect_eq(containsSubstr(rendered, "        api-->>user: 200 Cached\n"), true);
    expect_eq(containsSubstr(rendered, "    else cache miss\n"), true);
    expect_eq(containsSubstr(rendered, "        loop retry 3x\n"), true);
    expect_eq(containsSubstr(rendered, "            api->>worker: dispatch task\n"), true);
    expect_eq(containsSubstr(rendered, "            worker-->api: result\n"), true);
    expect_eq(containsSubstr(rendered, "        end\n"), true);
    expect_eq(containsSubstr(rendered, "        Note over worker: Done<br/>Logged\n"), true);
    expect_eq(containsSubstr(rendered, "    end\n"), true);
    expect_eq(containsSubstr(rendered, "    deactivate api\n"), true);
}

fn testErDiagramAttributesAndRelations()! {
    const alloc = std.testing.allocator;
    let er = ErBuilder.init(alloc);
    defer er.deinit();

    er.addEntity("USER")?;
    er.addAttribute("USER", "uuid", "id", "PK")?;
    er.addAttribute("USER", "string", "email", "UK")?;

    er.addEntity("ORDER")?;
    er.addAttribute("ORDER", "uuid", "id", "PK")?;
    er.addAttribute("ORDER", "uuid", "user_id", "FK")?;

    er.addRelation("USER", .OneToOne, "ORDER", .ZeroOrMany, "places")?;
    er.addRelation("ORDER", .OneToMany, "ITEM", .OneToMany, "contains (1+)")?;

    const rendered = er.render(alloc)?;
    defer alloc.free(rendered);

    expect_eq(containsSubstr(rendered, "erDiagram\n"), true);
    expect_eq(containsSubstr(rendered, "    USER {\n        uuid id PK\n        string email UK\n    }\n"), true);
    expect_eq(containsSubstr(rendered, "    ORDER {\n        uuid id PK\n        uuid user_id FK\n    }\n"), true);
    expect_eq(containsSubstr(rendered, "    USER ||--o{ ORDER : places\n"), true);
    expect_eq(containsSubstr(rendered, "    ORDER }|--|{ ITEM : \"contains (1+)\"\n"), true);
}

fn testStateAndErrorGuardsZeroLeak()! {
    const alloc = std.testing.allocator;

    // 1. StateBuilder composite & [*] transitions
    {
        let st = StateBuilder.init(alloc);
        defer st.deinit();

        st.addState("idle", "Idle (Ready)")?;
        st.addTransition("[*]", "idle", "")?;
        st.beginComposite("running", "Active Phase")?;
        st.addTransition("idle", "running", "start")?;
        st.noteRightOf("running", "Processing")?;
        st.endComposite()?;
        st.addTransition("running", "[*]", "complete")?;

        const rendered = st.render(alloc)?;
        defer alloc.free(rendered);

        expect_eq(containsSubstr(rendered, "stateDiagram-v2\n"), true);
        expect_eq(containsSubstr(rendered, "    state \"Idle (Ready)\" as idle\n"), true);
        expect_eq(containsSubstr(rendered, "    [*] --> idle\n"), true);
        expect_eq(containsSubstr(rendered, "    state \"Active Phase\" as running {\n"), true);
        expect_eq(containsSubstr(rendered, "        note right of running: Processing\n"), true);
        expect_eq(containsSubstr(rendered, "    running --> [*]: complete\n"), true);
    }

    // 2. InvalidIdentifier & DuplicateId & UnknownNode & EmptyDiagram & UnbalancedBlock
    expect_eq(isValidIdentifier(""), false);
    expect_eq(isValidIdentifier("bad id"), false);
    expect_eq(isValidIdentifier("good_id-1"), true);

    {
        let fc = FlowchartBuilder.init(alloc, .LR);
        defer fc.deinit();

        switch (fc.render(alloc)) {
            buf => {
                alloc.free(buf);
                expect_eq(true, false);
            },
            err(e) => expect_eq(e == MermaidError.EmptyDiagram, true),
        }

        switch (fc.addNode("", "Empty", .Rect)) {
            err(e) => expect_eq(e == MermaidError.InvalidIdentifier, true),
            else => expect_eq(true, false),
        }

        switch (fc.addNode("bad id", "Space", .Rect)) {
            err(e) => expect_eq(e == MermaidError.InvalidIdentifier, true),
            else => expect_eq(true, false),
        }

        fc.addNode("dup", "First", .Rect)?;
        switch (fc.addNode("dup", "Second", .Rect)) {
            err(e) => expect_eq(e == MermaidError.DuplicateId, true),
            else => expect_eq(true, false),
        }

        fc.setStrict(true);
        switch (fc.addEdge("dup", "missing_node", "", .SolidArrow)) {
            err(e) => expect_eq(e == MermaidError.UnknownNode, true),
            else => expect_eq(true, false),
        }

        fc.beginSubgraph("sg1", "Unclosed")?;
        switch (fc.render(alloc)) {
            buf => {
                alloc.free(buf);
                expect_eq(true, false);
            },
            err(e) => expect_eq(e == MermaidError.UnbalancedBlock, true),
        }
    }

    {
        let er = ErBuilder.init(alloc);
        defer er.deinit();
        switch (er.addAttribute("MISSING", "i32", "id", "PK")) {
            err(e) => expect_eq(e == MermaidError.UnknownNode, true),
            else => expect_eq(true, false),
        }
    }
}

fn testDynamicCapMermaidNamespace()! {
    const alloc = std.testing.allocator;
    let fc = cap.mermaid.FlowchartBuilder.init(alloc, cap.mermaid.FlowDirection.LR);
    defer fc.deinit();

    fc.addNode("a", "Agent (A)", cap.mermaid.NodeShape.Rounded)?;
    fc.addNode("b", "Tool (B)", cap.mermaid.NodeShape.Rect)?;
    fc.addEdge("a", "b", "invoke", cap.mermaid.EdgeStyle.SolidArrow)?;

    const out = fc.render(alloc)?;
    defer alloc.free(out);
    expect_eq(containsSubstr(out, "flowchart LR\n"), true);
    expect_eq(containsSubstr(out, "a(\"Agent (A)\")"), true);
    expect_eq(containsSubstr(out, "a -->|invoke| b"), true);
}

test("mermaid: FlowchartBuilder multi-shape, subgraph, edge styles & auto-quoting", () => {
    testFlowchartAndEscaping() catch unreachable;
});

test("mermaid: SequenceBuilder participants, arrows, activate & alt/else/loop/end", () => {
    testSequenceDiagramControlBlocks() catch unreachable;
});

test("mermaid: ErBuilder entity attributes (PK/FK/UK) & cardinality relations", () => {
    testErDiagramAttributesAndRelations() catch unreachable;
});

test("mermaid: StateBuilder, error guards (InvalidIdentifier/DuplicateId/UnknownNode) & zero leak", () => {
    testStateAndErrorGuardsZeroLeak() catch unreachable;
});

test("mermaid: dynamic cap.mermaid namespace mounting", () => {
    testDynamicCapMermaidNamespace() catch unreachable;
});

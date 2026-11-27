// Type definitions for CP

export enum EstreeNodeKind {
    Program,
    Identifier,
    Literal,
    BinaryExpression,
    CallExpression,
    FunctionDeclaration,
    BlockStatement,
    ReturnStatement,
    VariableDeclaration
}

export enum WalkPhase {
    Enter,
    Leave
}

export class WalkEvent {
    phase: WalkPhase,
    node_id: i32,
    parent_id: i32,
    depth: usize,
}

export class EstreeNode {
    kind: EstreeNodeKind,
    name: string,
    first_child: i32,
    next_sibling: i32,
    parent: i32,
    removed: bool,
}

export class EstwFrame {
    node_id: i32,
    next_child: i32,
    entered: bool,
    skipped: bool,
}

export fn estwValidId(id: i32, count: usize): bool;
export class EstreeArena {
    count: usize,
    nodes: [64]EstreeNode,
    fn init(): EstreeArena;
    fn addNode(_: auto, kind: EstreeNodeKind, name: string): i32;
    fn getNode(id: i32): EstreeNode?;
    fn appendChild(_: auto, parent: i32, child: i32): bool;
    fn removeNode(_: auto, id: i32): bool;
    fn replaceNode(_: auto, old_id: i32, new_id: i32): bool;
}

export class EstreeWalker {
    sp: usize,
    frames: [64]EstwFrame,
    fn init(root_id: i32): EstreeWalker;
    fn skip(_: auto);
    fn nextEvent(_: auto, tree: EstreeArena): WalkEvent?;
}


// packages/@cap/estree-walker/src/index.cp
// Clean-room zero-allocation index-pooled ESTree AST & DFS walker for CP.
// Parity with npm:estree-walker (Rich Harris).

pub const ESTW_MAX_NODES: usize = 64;
pub const ESTW_NONE: i32 = -1;

pub enum EstreeNodeKind {
    Program,
    Identifier,
    Literal,
    BinaryExpression,
    CallExpression,
    FunctionDeclaration,
    BlockStatement,
    ReturnStatement,
    VariableDeclaration,
}

pub enum WalkPhase {
    Enter,
    Leave,
}

pub struct WalkEvent {
    phase: WalkPhase,
    node_id: i32,
    parent_id: i32,
    depth: usize,
}

pub struct EstreeNode {
    kind: EstreeNodeKind,
    name: string,
    first_child: i32,
    next_sibling: i32,
    parent: i32,
    removed: bool,
}

struct EstwFrame {
    node_id: i32,
    next_child: i32,
    entered: bool,
    skipped: bool,
}

fn estwValidId(id: i32, count: usize): bool {
    return id >= 0 && usize(id) < count;
}

pub struct EstreeArena {
    count: usize,
    nodes: [64]EstreeNode,

    static fn init(): EstreeArena {
        let empty: [64]EstreeNode = undefined;
        let a = EstreeArena{
            count: 0,
            nodes: empty,
        };
        return a;
    }

    fn addNode(*_, kind: EstreeNodeKind, name: string): i32 {
        if (_.count >= ESTW_MAX_NODES) {
            return ESTW_NONE;
        }
        const id = i32(_.count);
        _.nodes[_.count] = EstreeNode{
            kind: kind,
            name: name,
            first_child: ESTW_NONE,
            next_sibling: ESTW_NONE,
            parent: ESTW_NONE,
            removed: false,
        };
        _.count += 1;
        return id;
    }

    fn getNode(id: i32): EstreeNode? {
        if (!estwValidId(id, _.count)) {
            return null;
        }
        return _.nodes[usize(id)];
    }

    fn appendChild(*_, parent: i32, child: i32): bool {
        if (!estwValidId(parent, _.count) || !estwValidId(child, _.count) || parent == child) {
            return false;
        }
        const p_idx = usize(parent);
        const c_idx = usize(child);
        if (_.nodes[p_idx].removed || _.nodes[c_idx].removed) {
            return false;
        }
        _.nodes[c_idx].parent = parent;
        _.nodes[c_idx].next_sibling = ESTW_NONE;
        if (_.nodes[p_idx].first_child == ESTW_NONE) {
            _.nodes[p_idx].first_child = child;
            return true;
        }
        let cur = _.nodes[p_idx].first_child;
        let guard: usize = 0;
        while (estwValidId(cur, _.count) && guard < ESTW_MAX_NODES) {
            const next = _.nodes[usize(cur)].next_sibling;
            if (next == ESTW_NONE) {
                _.nodes[usize(cur)].next_sibling = child;
                return true;
            }
            cur = next;
            guard += 1;
        }
        return false;
    }

    fn removeNode(*_, id: i32): bool {
        if (!estwValidId(id, _.count)) {
            return false;
        }
        const idx = usize(id);
        if (_.nodes[idx].removed) {
            return false;
        }
        const p = _.nodes[idx].parent;
        if (estwValidId(p, _.count)) {
            const p_idx = usize(p);
            if (_.nodes[p_idx].first_child == id) {
                _.nodes[p_idx].first_child = _.nodes[idx].next_sibling;
            } else {
                let cur = _.nodes[p_idx].first_child;
                let guard: usize = 0;
                while (estwValidId(cur, _.count) && guard < ESTW_MAX_NODES) {
                    if (_.nodes[usize(cur)].next_sibling == id) {
                        _.nodes[usize(cur)].next_sibling = _.nodes[idx].next_sibling;
                        break;
                    }
                    cur = _.nodes[usize(cur)].next_sibling;
                    guard += 1;
                }
            }
        }
        _.nodes[idx].parent = ESTW_NONE;
        _.nodes[idx].next_sibling = ESTW_NONE;
        _.nodes[idx].removed = true;
        return true;
    }

    fn replaceNode(*_, old_id: i32, new_id: i32): bool {
        if (!estwValidId(old_id, _.count) || !estwValidId(new_id, _.count) || old_id == new_id) {
            return false;
        }
        const o_idx = usize(old_id);
        const n_idx = usize(new_id);
        if (_.nodes[o_idx].removed || _.nodes[n_idx].removed) {
            return false;
        }
        const p = _.nodes[o_idx].parent;
        const old_next = _.nodes[o_idx].next_sibling;
        if (estwValidId(p, _.count)) {
            const p_idx = usize(p);
            if (_.nodes[p_idx].first_child == old_id) {
                _.nodes[p_idx].first_child = new_id;
            } else {
                let cur = _.nodes[p_idx].first_child;
                let guard: usize = 0;
                while (estwValidId(cur, _.count) && guard < ESTW_MAX_NODES) {
                    if (_.nodes[usize(cur)].next_sibling == old_id) {
                        _.nodes[usize(cur)].next_sibling = new_id;
                        break;
                    }
                    cur = _.nodes[usize(cur)].next_sibling;
                    guard += 1;
                }
            }
        }
        _.nodes[n_idx].parent = p;
        _.nodes[n_idx].next_sibling = old_next;
        _.nodes[o_idx].parent = ESTW_NONE;
        _.nodes[o_idx].next_sibling = ESTW_NONE;
        _.nodes[o_idx].removed = true;
        return true;
    }
}

pub struct EstreeWalker {
    sp: usize,
    frames: [64]EstwFrame,

    static fn init(root_id: i32): EstreeWalker {
        let empty: [64]EstwFrame = undefined;
        let w = EstreeWalker{
            sp: 0,
            frames: empty,
        };
        if (root_id >= 0) {
            w.frames[0] = EstwFrame{
                node_id: root_id,
                next_child: ESTW_NONE,
                entered: false,
                skipped: false,
            };
            w.sp = 1;
        }
        return w;
    }

    fn skip(*_) {
        if (_.sp > 0) {
            _.frames[_.sp - 1].skipped = true;
        }
    }

    fn nextEvent(*_, tree: EstreeArena): WalkEvent? {
        while (_.sp > 0) {
            const top = _.sp - 1;
            const cur_id = _.frames[top].node_id;
            if (!estwValidId(cur_id, tree.count) || tree.nodes[usize(cur_id)].removed) {
                _.sp -= 1;
                continue;
            }
            const node = tree.nodes[usize(cur_id)];
            if (!_.frames[top].entered) {
                _.frames[top].entered = true;
                _.frames[top].next_child = node.first_child;
                return WalkEvent{
                    phase: .Enter,
                    node_id: cur_id,
                    parent_id: node.parent,
                    depth: top,
                };
            }
            if (!_.frames[top].skipped) {
                let child = _.frames[top].next_child;
                while (estwValidId(child, tree.count) && tree.nodes[usize(child)].removed) {
                    child = tree.nodes[usize(child)].next_sibling;
                }
                if (estwValidId(child, tree.count)) {
                    _.frames[top].next_child = tree.nodes[usize(child)].next_sibling;
                    if (_.sp < ESTW_MAX_NODES) {
                        _.frames[_.sp] = EstwFrame{
                            node_id: child,
                            next_child: ESTW_NONE,
                            entered: false,
                            skipped: false,
                        };
                        _.sp += 1;
                        continue;
                    }
                }
            }
            _.sp -= 1;
            return WalkEvent{
                phase: .Leave,
                node_id: cur_id,
                parent_id: node.parent,
                depth: top,
            };
        }
        return null;
    }
}

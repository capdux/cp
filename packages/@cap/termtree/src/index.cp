// @cap/termtree — Zero-allocation CLI tree structure renderer
// Clean-room parity with crates.io:termtree (MIT)

pub struct GlyphPalette {
    middle_item: string,
    last_item: string,
    middle_skip: string,
    last_skip: string,

    static fn unicode(): GlyphPalette {
        return GlyphPalette{
            middle_item: "├── ",
            last_item: "└── ",
            middle_skip: "│   ",
            last_skip: "    ",
        };
    }

    static fn ascii(): GlyphPalette {
        return GlyphPalette{
            middle_item: "|-- ",
            last_item: "`-- ",
            middle_skip: "|   ",
            last_skip: "    ",
        };
    }
}

pub struct TermTreeNode {
    label: string,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
}

fn ttrAppendBytes(out: []u8, pos: usize, src: string): usize? {
    if (pos + src.len > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        out[pos + i] = src[i];
    }
    return pos + src.len;
}

fn ttrAppendByte(out: []u8, pos: usize, b: u8): usize? {
    if (pos >= out.len) {
        return null;
    }
    out[pos] = b;
    return pos + 1;
}

pub struct TermTreeArena {
    nodes: [64]TermTreeNode,
    count: usize,

    static fn init(): TermTreeArena {
        let arr: [64]TermTreeNode = undefined;
        let i: usize = 0;
        while (i < 64) : (i += 1) {
            arr[i] = TermTreeNode{
                label: "",
                first_child: -1,
                last_child: -1,
                next_sibling: -1,
            };
        }
        return TermTreeArena{
            nodes: arr,
            count: 0,
        };
    }

    fn newRoot(*_, label: string): usize {
        if (_.count >= 64) {
            return 64;
        }
        const idx = _.count;
        _.nodes[idx] = TermTreeNode{
            label: label,
            first_child: -1,
            last_child: -1,
            next_sibling: -1,
        };
        _.count += 1;
        return idx;
    }

    fn addChild(*_, parent: usize, label: string): i32 {
        if (parent >= _.count || _.count >= 64) {
            return -1;
        }
        const idx = _.count;
        _.nodes[idx] = TermTreeNode{
            label: label,
            first_child: -1,
            last_child: -1,
            next_sibling: -1,
        };
        _.count += 1;

        const prev_last = _.nodes[parent].last_child;
        if (prev_last < 0) {
            _.nodes[parent].first_child = i32(idx);
            _.nodes[parent].last_child = i32(idx);
        } else {
            _.nodes[usize(prev_last)].next_sibling = i32(idx);
            _.nodes[parent].last_child = i32(idx);
        }
        return i32(idx);
    }

    fn nodeCount(): usize {
        return _.count;
    }

    fn maxDepth(root: usize): usize {
        if (root >= _.count) {
            return 0;
        }
        let stack_node: [64]usize = undefined;
        let stack_depth: [64]usize = undefined;
        let sp: usize = 0;
        stack_node[0] = root;
        stack_depth[0] = 1;
        sp = 1;

        let best: usize = 1;
        while (sp > 0) {
            sp -= 1;
            const cur = stack_node[sp];
            const dep = stack_depth[sp];
            if (dep > best) {
                best = dep;
            }
            let ch = _.nodes[cur].first_child;
            while (ch >= 0) {
                const cidx = usize(ch);
                if (sp < 64) {
                    stack_node[sp] = cidx;
                    stack_depth[sp] = dep + 1;
                    sp += 1;
                }
                ch = _.nodes[cidx].next_sibling;
            }
        }
        return best;
    }

    fn renderWithGlyphs(root: usize, g: GlyphPalette, out: []u8): usize {
        if (root >= _.count) {
            return 0;
        }
        let pos: usize = 0;
        if (p0 := ttrAppendBytes(out, pos, _.nodes[root].label)) {
            pos = p0;
        } else {
            return 0;
        }
        if (p1 := ttrAppendByte(out, pos, '\n')) {
            pos = p1;
        } else {
            return 0;
        }

        let frame_child: [64]i32 = undefined;
        let sp: usize = 0;
        if (_.nodes[root].first_child >= 0) {
            frame_child[0] = _.nodes[root].first_child;
            sp = 1;
        }

        while (sp > 0) {
            const cur_child = frame_child[sp - 1];
            if (cur_child < 0) {
                sp -= 1;
                continue;
            }
            const cidx = usize(cur_child);
            const next_sib = _.nodes[cidx].next_sibling;
            frame_child[sp - 1] = next_sib;
            const is_last: bool = next_sib < 0;

            let d: usize = 0;
            while (d + 1 < sp) : (d += 1) {
                const skip_glyph = if (frame_child[d] >= 0) g.middle_skip else g.last_skip;
                if (ps := ttrAppendBytes(out, pos, skip_glyph)) {
                    pos = ps;
                } else {
                    return 0;
                }
            }

            const item_glyph = if (is_last) g.last_item else g.middle_item;
            if (pi := ttrAppendBytes(out, pos, item_glyph)) {
                pos = pi;
            } else {
                return 0;
            }
            if (pl := ttrAppendBytes(out, pos, _.nodes[cidx].label)) {
                pos = pl;
            } else {
                return 0;
            }
            if (pn := ttrAppendByte(out, pos, '\n')) {
                pos = pn;
            } else {
                return 0;
            }

            if (_.nodes[cidx].first_child >= 0 && sp < 64) {
                frame_child[sp] = _.nodes[cidx].first_child;
                sp += 1;
            }
        }
        return pos;
    }

    fn render(root: usize, out: []u8): usize {
        return _.renderWithGlyphs(root, GlyphPalette.unicode(), out);
    }
}

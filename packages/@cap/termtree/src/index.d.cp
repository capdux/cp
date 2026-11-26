// Type definitions for CP

export class GlyphPalette {
    middle_item: string,
    last_item: string,
    middle_skip: string,
    last_skip: string,
    fn unicode(): GlyphPalette;
    fn ascii(): GlyphPalette;
}

export class TermTreeNode {
    label: string,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
}

export fn ttrAppendBytes(out: []u8, pos: usize, src: string): usize?;
export fn ttrAppendByte(out: []u8, pos: usize, b: u8): usize?;
export class TermTreeArena {
    nodes: [64]TermTreeNode,
    count: usize,
    fn init(): TermTreeArena;
    fn newRoot(_: auto, label: string): usize;
    fn addChild(_: auto, parent: usize, label: string): i32;
    fn nodeCount(): usize;
    fn maxDepth(root: usize): usize;
    fn renderWithGlyphs(root: usize, g: GlyphPalette, out: []u8): usize;
    fn render(root: usize, out: []u8): usize;
}


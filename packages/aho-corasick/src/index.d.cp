// Type definitions for CP

export class AcMatch {
    pattern_id: u16,
    start: usize,
    end: usize,
    found: bool,
}

export fn noMatch(): AcMatch;
export class AhoCorasick {
    edge_char: [MAX_NODES]u8,
    first_child: [MAX_NODES]u16,
    next_sibling: [MAX_NODES]u16,
    fail: [MAX_NODES]u16,
    dict_link: [MAX_NODES]u16,
    out_pat: [MAX_NODES]i16,
    pat_lens: [MAX_PATTERNS]usize,
    node_count: usize,
    pat_count: usize,
    built: bool,
    fn init(): AhoCorasick;
    fn findChild(node: u16, ch: u8): u16;
    fn addPattern(_: auto, pat: &[]u8): i32;
    fn build(_: auto): bool;
    fn patternCount(): usize;
    fn stepState(state_in: u16, ch: u8): u16;
    fn isMatch(text: &[]u8): bool;
    fn findFirst(text: &[]u8): AcMatch;
    fn findAll(text: &[]u8, out: []AcMatch): usize;
    fn countMatches(text: &[]u8): usize;
}


import {
    ENTRY_ANY, ENTRY_FILE, ENTRY_DIR,
    LocateOptions, FsEntry, locatePathInEntries, locateAllPathsInEntries
} from "../../locate-path/src/index.cp";

pub fn pathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool {
    let cands: [1]string = [target];
    let opts = LocateOptions.defaults(cwd);
    opts.entry_type = ENTRY_ANY;
    let buf: [256]u8 = [0; 256];
    return locatePathInEntries(cands[0..1], entries, opts, buf[0..256]) != null;
}

pub fn fileExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool {
    let cands: [1]string = [target];
    let opts = LocateOptions.defaults(cwd);
    opts.entry_type = ENTRY_FILE;
    let buf: [256]u8 = [0; 256];
    return locatePathInEntries(cands[0..1], entries, opts, buf[0..256]) != null;
}

pub fn dirExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool {
    let cands: [1]string = [target];
    let opts = LocateOptions.defaults(cwd);
    opts.entry_type = ENTRY_DIR;
    let buf: [256]u8 = [0; 256];
    return locatePathInEntries(cands[0..1], entries, opts, buf[0..256]) != null;
}

pub fn realPathExistsInEntries(target: string, cwd: string, entries: []FsEntry): bool {
    let cands: [1]string = [target];
    let opts = LocateOptions.defaults(cwd);
    opts.allow_symlinks = false;
    opts.entry_type = ENTRY_ANY;
    let buf: [256]u8 = [0; 256];
    return locatePathInEntries(cands[0..1], entries, opts, buf[0..256]) != null;
}

pub fn countExistingPaths(targets: []string, cwd: string, entries: []FsEntry, out_indices: []usize): usize {
    let opts = LocateOptions.defaults(cwd);
    opts.entry_type = ENTRY_ANY;
    return locateAllPathsInEntries(targets, entries, opts, out_indices);
}

import os

# Lineage Mapping based on our audit
LINEAGE_MAP = {
    "lib/sort/index.cp": "// Source: Zig Standard Library: std.sort",
    "lib/hash/index.cp": "// Source: Zig Standard Library: std.hash",
    "lib/pack/tar.cp": "// Source: Bun src/libarchive/libarchive.cp",
    "lib/col/priority_queue.cp": "// Source: Custom implementation by Antigravity AI",
    "lib/bigint/index.cp": "// Source: Derived from Bun & Zig std.math.big",
    "lib/sys/index.cp": "// Source: Zig Standard Library: std.posix",
    # ... more to be added as we scan ...
}

def annotate_file(filepath):
    with open(filepath, 'r') as f:
        lines = f.readlines()
    
    if not lines:
        return
        
    first_line = lines[0].strip()
    
    # Check if already annotated
    if first_line.startswith("// Source:"):
        return

    # Determine annotation
    annotation = LINEAGE_MAP.get(filepath, "// Source: Custom implementation by Antigravity AI")
    
    # Write back with annotation
    with open(filepath, 'w') as f:
        f.write(annotation + "\n")
        f.writelines(lines)

# Process all files
with open("lib_files.txt", "r") as f:
    for line in f:
        filepath = line.strip()
        if os.path.exists(filepath):
            annotate_file(filepath)

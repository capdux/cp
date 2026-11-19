
const std = @import("std");

pub const ImportScanner = struct {
    allocator: std.mem.Allocator,
    
    pub fn init(allocator: std.mem.Allocator) ImportScanner {
        return ImportScanner{ .allocator = allocator };
    }

    pub fn scan(self: *ImportScanner, start_file: []const u8) !std.ArrayList([]const u8) {
        var visited = std.StringHashMap(void).init(self.allocator);
        defer visited.deinit();
        var queue = std.ArrayList([]const u8).init(self.allocator);
        defer queue.deinit();
        
        try queue.append(start_file);
        
        var results = std.ArrayList([]const u8).init(self.allocator);
        
        while (queue.items.len > 0) {
            const current = queue.orderedRemove(0);
            if (visited.contains(current)) continue;
            try visited.put(current, {});
            try results.append(current);
            
            // 扫描导入
            const imports = try self.parseImports(current);
            defer imports.deinit();
            
            for (imports.items) |imp| {
                if (!visited.contains(imp)) {
                    try queue.append(try self.allocator.dupe(u8, imp));
                }
            }
        }
        return results;
    }

    fn parseImports(self: *ImportScanner, file_path: []const u8) !std.ArrayList([]const u8) {
        var imports = std.ArrayList([]const u8).init(self.allocator);
        
        var file = try std.fs.openFileAbsolute(file_path, .{});
        defer file.close();
        
        var reader = std.io.bufferedReader(file.reader());
        var buf: [1024]u8 = undefined;
        
        while (try reader.reader().readUntilDelimiterOrEof(&buf, '\n')) |line| {
            if (std.mem.indexOf(u8, line, "import")) |_| {
                if (std.mem.indexOf(u8, line, "from")) |from_idx| {
                    const start_quote = std.mem.indexOfScalar(u8, line[from_idx..], '"') orelse std.mem.indexOfScalar(u8, line[from_idx..], '\'') orelse continue;
                    const end_quote = std.mem.lastIndexOfScalar(u8, line, '"') orelse std.mem.lastIndexOfScalar(u8, line, '\'') orelse continue;
                    
                    if (start_quote + from_idx < end_quote) {
                        try imports.append(try self.allocator.dupe(u8, line[start_quote + from_idx + 1 .. end_quote]));
                    }
                }
            }
        }
        return imports;
    }
};

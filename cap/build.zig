const std = @import("std");

pub fn build(b: *std.Build) void {
    const native_target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const root_mod = b.createModule(.{
        .root_source_file = b.path("src/main.zig"),
        .target = native_target,
        .optimize = optimize,
        .link_libc = true,
    });
    root_mod.addIncludePath(b.path("../packages/zstd/c"));
    root_mod.addCSourceFile(.{
        .file = b.path("../packages/zstd/c/zstd.c"),
        .flags = &.{ "-O2", "-DZSTD_MULTITHREAD=0" },
    });
    // Statically link libdeflate (MIT) for zero-CLI RFC 1952 gzip (.tgz) compression & decompression
    root_mod.addIncludePath(b.path("../vendor/libdeflate"));
    root_mod.addIncludePath(b.path("../vendor/libdeflate/lib"));
    const deflate_flags: []const []const u8 = &.{ "-O2", "-std=c99" };
    root_mod.addCSourceFiles(.{
        .root = b.path("../vendor/libdeflate/lib"),
        .files = &.{
            "adler32.c",
            "crc32.c",
            "deflate_compress.c",
            "deflate_decompress.c",
            "gzip_compress.c",
            "gzip_decompress.c",
            "utils.c",
            "x86/cpu_features.c",
            "arm/cpu_features.c",
        },
        .flags = deflate_flags,
    });

    const exe = b.addExecutable(.{
        .name = "cap",
        .root_module = root_mod,
    });

    const install_artifact = b.addInstallArtifact(exe, .{ .dest_dir = .{ .override = .{ .custom = "" } } });
    b.getInstallStep().dependOn(&install_artifact.step);

    const bin_install = b.addInstallArtifact(exe, .{ .dest_dir = .{ .override = .{ .custom = "../bin" } } });
    b.getInstallStep().dependOn(&bin_install.step);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}

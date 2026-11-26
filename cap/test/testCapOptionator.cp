import { test, expect_eq } from "cap:test";
import {
    OptionSpec,
    formatOptionRow,
    generateHelp,
    findOptionSpec,
} from "../../packages/@cap/optionator/src/index.cp";

fn main() {
    test("1. @cap/optionator — formatOptionRow aligns short, long, type, description, and default", () => {
        const spec = OptionSpec{
            long_name: "port",
            short_alias: "p",
            value_type: "Int",
            description: "Server port",
            default_val: "8080",
            required: false,
        };
        let buf: [256]u8 = [0; 256];
        const n = formatOptionRow(spec, 22, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "  -p, --port Int      Server port (default: 8080)",
        );
    });

    test("2. @cap/optionator — formatOptionRow supports long-only required option", () => {
        const spec = OptionSpec{
            long_name: "config",
            short_alias: "",
            value_type: "Path",
            description: "Config file",
            default_val: "",
            required: true,
        };
        let buf: [256]u8 = [0; 256];
        const n = formatOptionRow(spec, 20, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "  --config Path     Config file [required]",
        );
    });

    test("3. @cap/optionator — formatOptionRow supports short-only boolean flag", () => {
        const spec = OptionSpec{
            long_name: "",
            short_alias: "v",
            value_type: "",
            description: "Verbose output",
            default_val: "",
            required: false,
        };
        let buf: [256]u8 = [0; 256];
        const n = formatOptionRow(spec, 10, buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "  -v      Verbose output",
        );
    });

    test("4. @cap/optionator — generateHelp auto-computes alignment across options", () => {
        const specs: [3]OptionSpec = [
            OptionSpec{
                long_name: "port",
                short_alias: "p",
                value_type: "Int",
                description: "Server port",
                default_val: "8080",
                required: false,
            },
            OptionSpec{
                long_name: "output-dir",
                short_alias: "o",
                value_type: "String",
                description: "Output directory",
                default_val: "",
                required: true,
            },
            OptionSpec{
                long_name: "help",
                short_alias: "h",
                value_type: "",
                description: "Show help",
                default_val: "",
                required: false,
            },
        ];
        let buf: [512]u8 = [0; 512];
        const n = generateHelp("mycli", "Fast build tool", specs[0..3], buf[0..512]);
        expect_eq(
            buf[0..n] as string,
            "Usage: mycli [options]\nFast build tool\n\nOptions:\n  -p, --port Int           Server port (default: 8080)\n  -o, --output-dir String  Output directory [required]\n  -h, --help               Show help\n",
        );
    });

    test("5. @cap/optionator — findOptionSpec resolves both --long and -s flags", () => {
        const specs: [2]OptionSpec = [
            OptionSpec{
                long_name: "port",
                short_alias: "p",
                value_type: "Int",
                description: "Server port",
                default_val: "8080",
                required: false,
            },
            OptionSpec{
                long_name: "verbose",
                short_alias: "v",
                value_type: "",
                description: "Verbose logs",
                default_val: "",
                required: false,
            },
        ];
        if (s1 := findOptionSpec(specs[0..2], "--port")) {
            expect_eq(s1.short_alias, "p");
            expect_eq(s1.default_val, "8080");
        } else {
            expect_eq(true, false);
        }
        if (s2 := findOptionSpec(specs[0..2], "-v")) {
            expect_eq(s2.long_name, "verbose");
        } else {
            expect_eq(true, false);
        }
    });

    test("6. @cap/optionator — findOptionSpec returns null for unknown or malformed flags", () => {
        const specs: [1]OptionSpec = [
            OptionSpec{
                long_name: "port",
                short_alias: "p",
                value_type: "Int",
                description: "Server port",
                default_val: "8080",
                required: false,
            },
        ];
        expect_eq(findOptionSpec(specs[0..1], "--unknown") == null, true);
        expect_eq(findOptionSpec(specs[0..1], "-x") == null, true);
        expect_eq(findOptionSpec(specs[0..1], "port") == null, true);
        expect_eq(findOptionSpec(specs[0..1], "-") == null, true);
        expect_eq(findOptionSpec(specs[0..1], "--") == null, true);
    });
}

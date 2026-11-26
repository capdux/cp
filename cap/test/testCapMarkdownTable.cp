import { test, expect_eq } from "cap:test";
import {
    TableAlign,
    renderMarkdownTable,
} from "../../packages/@cap/markdown-table/src/index.cp";

fn main() {
    test("1. @cap/markdown-table — basic 2x2 table with None alignment", () => {
        const headers: [2]string = ["Name", "Age"];
        const rows: [4]string = ["Ada", "36", "Bob", "9"];
        const aligns: [2]TableAlign = [.None, .None];
        let buf: [256]u8 = [0; 256];
        const n = renderMarkdownTable(headers[0..2], rows[0..4], 2, 2, aligns[0..2], buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "| Name | Age |\n| ---- | --- |\n| Ada  | 36  |\n| Bob  | 9   |",
        );
    });

    test("2. @cap/markdown-table — all 4 alignments (None, Left, Center, Right)", () => {
        const headers: [4]string = ["A", "B", "C", "D"];
        const rows: [4]string = ["1", "2", "3", "4"];
        const aligns: [4]TableAlign = [.None, .Left, .Center, .Right];
        let buf: [256]u8 = [0; 256];
        const n = renderMarkdownTable(headers[0..4], rows[0..4], 4, 1, aligns[0..4], buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "| A   | B    |   C   |    D |\n| --- | :--- | :---: | ---: |\n| 1   | 2    |   3   |    4 |",
        );
    });

    test("3. @cap/markdown-table — auto-expands column width for long cells", () => {
        const headers: [2]string = ["ID", "Lang"];
        const rows: [4]string = ["1", "CP-Lang", "100", "Zig"];
        const aligns: [2]TableAlign = [.Right, .Center];
        let buf: [256]u8 = [0; 256];
        const n = renderMarkdownTable(headers[0..2], rows[0..4], 2, 2, aligns[0..2], buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "|   ID |  Lang   |\n| ---: | :-----: |\n|    1 | CP-Lang |\n|  100 |   Zig   |",
        );
    });

    test("4. @cap/markdown-table — missing aligns default to None", () => {
        const headers: [2]string = ["K", "V"];
        const rows: [2]string = ["x", "y"];
        const aligns: [1]TableAlign = [.Left];
        let buf: [256]u8 = [0; 256];
        const n = renderMarkdownTable(headers[0..2], rows[0..2], 2, 1, aligns[0..1], buf[0..256]);
        expect_eq(
            buf[0..n] as string,
            "| K    | V   |\n| :--- | --- |\n| x    | y   |",
        );
    });

    test("5. @cap/markdown-table — header-only and empty column edge cases", () => {
        const headers: [2]string = ["Col1", "Col2"];
        const dummy_rows: [1]string = [""];
        const dummy_aligns: [1]TableAlign = [.None];
        let buf: [256]u8 = [0; 256];
        const n1 = renderMarkdownTable(headers[0..2], dummy_rows[0..0], 2, 0, dummy_aligns[0..0], buf[0..256]);
        expect_eq(
            buf[0..n1] as string,
            "| Col1 | Col2 |\n| ---- | ---- |",
        );
        const n0 = renderMarkdownTable(headers[0..2], dummy_rows[0..0], 0, 0, dummy_aligns[0..0], buf[0..256]);
        expect_eq(n0, 0);
    });

    test("6. @cap/markdown-table — safely truncates when output buffer is small", () => {
        const headers: [2]string = ["Key", "Value"];
        const rows: [2]string = ["foo", "bar"];
        const aligns: [2]TableAlign = [.None, .None];
        let small: [12]u8 = [0; 12];
        const n = renderMarkdownTable(headers[0..2], rows[0..2], 2, 1, aligns[0..2], small[0..12]);
        expect_eq(n, 12);
        expect_eq(small[0..n] as string, "| Key | Valu");
    });
}

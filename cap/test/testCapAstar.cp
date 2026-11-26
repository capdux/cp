import { test, expect_eq } from "cap:test";
import { GridMap, GridPos } from "../../packages/@cap/astar/src/index.cp";

fn main() {
    test("open grid straight and L-shaped optimal cost", () => {
        let gm = GridMap.init(10, 10);
        let path: [64]GridPos = undefined;
        let res = gm.findPath(GridPos { x: 0, y: 0 }, GridPos { x: 4, y: 3 }, false, path[0..64]);
        expect_eq(res.found, true);
        expect_eq(res.cost, 70);
        expect_eq(res.path_len, 8);
        expect_eq(path[0].x, 0);
        expect_eq(path[0].y, 0);
        expect_eq(path[res.path_len - 1].x, 4);
        expect_eq(path[res.path_len - 1].y, 3);
    });

    test("detour around U-shaped wall obstacle", () => {
        let gm = GridMap.init(8, 8);
        gm.setBlocked(3, 1, true);
        gm.setBlocked(3, 2, true);
        gm.setBlocked(3, 3, true);
        gm.setBlocked(3, 4, true);
        gm.setBlocked(2, 1, true);
        gm.setBlocked(2, 4, true);
        let path: [64]GridPos = undefined;
        let res = gm.findPath(GridPos { x: 1, y: 2 }, GridPos { x: 5, y: 2 }, false, path[0..64]);
        expect_eq(res.found, true);
        expect_eq(res.cost > 40, true);
        expect_eq(path[res.path_len - 1].x, 5);
        expect_eq(path[res.path_len - 1].y, 2);
    });

    test("8-way diagonal path is shorter than 4-way", () => {
        let gm = GridMap.init(10, 10);
        let p4: [64]GridPos = undefined;
        let p8: [64]GridPos = undefined;
        let r4 = gm.findPath(GridPos { x: 0, y: 0 }, GridPos { x: 5, y: 5 }, false, p4[0..64]);
        let r8 = gm.findPath(GridPos { x: 0, y: 0 }, GridPos { x: 5, y: 5 }, true, p8[0..64]);
        expect_eq(r4.found, true);
        expect_eq(r8.found, true);
        expect_eq(r4.cost, 100);
        expect_eq(r8.cost, 70);
        expect_eq(r8.path_len, 6);
    });

    test("start equals goal returns cost 0 and length 1", () => {
        let gm = GridMap.init(5, 5);
        let path: [8]GridPos = undefined;
        let res = gm.findPath(GridPos { x: 2, y: 2 }, GridPos { x: 2, y: 2 }, true, path[0..8]);
        expect_eq(res.found, true);
        expect_eq(res.cost, 0);
        expect_eq(res.path_len, 1);
        expect_eq(path[0].x, 2);
        expect_eq(path[0].y, 2);
    });

    test("completely enclosed wall returns found == false", () => {
        let gm = GridMap.init(6, 6);
        let y: usize = 0;
        while (y < 6) {
            gm.setBlocked(3, y, true);
            y += 1;
        }
        let path: [32]GridPos = undefined;
        let res = gm.findPath(GridPos { x: 0, y: 2 }, GridPos { x: 5, y: 2 }, true, path[0..32]);
        expect_eq(res.found, false);
    });

    test("out-of-bounds and blocked start/goal are rejected", () => {
        let gm = GridMap.init(5, 5);
        gm.setBlocked(4, 4, true);
        let path: [16]GridPos = undefined;
        expect_eq(gm.findPath(GridPos { x: 0, y: 0 }, GridPos { x: 4, y: 4 }, false, path[0..16]).found, false);
        expect_eq(gm.findPath(GridPos { x: 0, y: 0 }, GridPos { x: 9, y: 0 }, false, path[0..16]).found, false);
        expect_eq(gm.isBlocked(4, 4), true);
        expect_eq(gm.isBlocked(9, 9), true);
    });
}

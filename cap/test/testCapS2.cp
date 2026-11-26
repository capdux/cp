import { test, expect_eq } from "cap:test";
import {
    S2_MAX_LEVEL, LatLng, S2CellId, haversineMeters
} from "../../packages/@cap/s2/src/index.cp";

fn main() {
    test("1. @cap/s2 — S2CellId.fromFace creates valid level-0 face cells", () => {
        let f0 = S2CellId.fromFace(0) ?? S2CellId { id: 0 };
        expect_eq(f0.isValid(), true);
        expect_eq(f0.face(), 0);
        expect_eq(f0.level(), 0);
        expect_eq(f0.isLeaf(), false);
        expect_eq(S2CellId.fromFace(6) == null, true);
    });

    test("2. @cap/s2 — child() and parent() hierarchy roundtrip", () => {
        let root = S2CellId.fromFace(3) ?? S2CellId { id: 0 };
        let c1 = root.child(2) ?? S2CellId { id: 0 };
        expect_eq(c1.isValid(), true);
        expect_eq(c1.face(), 3);
        expect_eq(c1.level(), 1);

        let c2 = c1.child(1) ?? S2CellId { id: 0 };
        expect_eq(c2.level(), 2);

        let p1 = c2.parent(1) ?? S2CellId { id: 0 };
        expect_eq(p1.id, c1.id);
        let p0 = c2.parent(0) ?? S2CellId { id: 0 };
        expect_eq(p0.id, root.id);
    });

    test("3. @cap/s2 — contains() checks Hilbert cell range enclosure", () => {
        let root = S2CellId.fromFace(1) ?? S2CellId { id: 0 };
        let c0 = root.child(0) ?? S2CellId { id: 0 };
        let c3 = root.child(3) ?? S2CellId { id: 0 };
        let other_face = S2CellId.fromFace(2) ?? S2CellId { id: 0 };

        expect_eq(root.contains(c0), true);
        expect_eq(root.contains(c3), true);
        expect_eq(c0.contains(c3), false);
        expect_eq(root.contains(other_face), false);
    });

    test("4. @cap/s2 — leaf cell at level 30 has isLeaf()=true & rejects further child", () => {
        let leaf = S2CellId.fromFacePosLevel(4, 123456, S2_MAX_LEVEL) ?? S2CellId { id: 0 };
        expect_eq(leaf.isValid(), true);
        expect_eq(leaf.level(), 30);
        expect_eq(leaf.isLeaf(), true);
        expect_eq(leaf.child(0) == null, true);
    });

    test("5. @cap/s2 — LatLng validation bounds", () => {
        expect_eq(LatLng.fromDegrees(-33.8688, 151.2093).isValid(), true);
        expect_eq(LatLng.fromDegrees(91.0, 0.0).isValid(), false);
        expect_eq(LatLng.fromDegrees(0.0, -181.0).isValid(), false);
    });

    test("6. @cap/s2 — haversineMeters computes great-circle distance accurately", () => {
        let a = LatLng.fromDegrees(0.0, 0.0);
        let b = LatLng.fromDegrees(0.0, 1.0);
        // 1 degree on equator ~= 111.195 km
        let d = haversineMeters(a, b);
        expect_eq(d > 111000.0 && d < 111400.0, true);
        expect_eq(haversineMeters(a, a) < 1.0, true);
    });
}

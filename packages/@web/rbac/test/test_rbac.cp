// Regression Test Suite for `@web/rbac`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import { RbacConfig, registerRbac, rbacGrant, rbacProtect, rbacCan, rbacReset } from "../src/index.cp";

fn hOrdersDelete(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"deleted\":true}");
}

fn hOrdersList(ctx: *Context): Response {
    * = ctx;
    return Response.json(200, "{\"items\":[]}");
}

fn main() {
    rbacReset();
    * = rbacGrant("admin", "*");
    * = rbacGrant("ops", "orders:*");
    * = rbacGrant("viewer", "orders:read");
    * = rbacProtect(AppMethod.GET, "/orders", "orders:read");
    * = rbacProtect(AppMethod.DELETE, "/orders", "orders:delete");

    if (!rbacCan("admin", "billing:refund") || !rbacCan("ops", "orders:delete") || rbacCan("viewer", "orders:delete")) {
        @panic("Test 1 failed: rbacCan wildcard evaluation");
    }

    let app = App.init();
    * = registerRbac(*app, RbacConfig{ role_header: "X-Role", default_role: "guest" });
    * = app.get("/orders", hOrdersList).delete("/orders", hOrdersDelete);

    // Guest denied (403)
    if (app.dispatch(Request.init(AppMethod.GET, "/orders")).status != 403) {
        @panic("Test 2 failed: guest should be 403");
    }

    // Viewer can GET /orders (200) but cannot DELETE /orders (403)
    let vr = Request.init(AppMethod.GET, "/orders");
    * = vr.withHeader("X-Role", "viewer");
    if (app.dispatch(vr).status != 200) @panic("Test 3a failed: viewer GET expected 200");

    let vd = Request.init(AppMethod.DELETE, "/orders");
    * = vd.withHeader("X-Role", "viewer");
    if (app.dispatch(vd).status != 403) @panic("Test 3b failed: viewer DELETE expected 403");

    // Ops can DELETE /orders via `orders:*` wildcard (200)
    let od = Request.init(AppMethod.DELETE, "/orders");
    * = od.withHeader("X-Role", "ops");
    if (app.dispatch(od).status != 200) @panic("Test 4 failed: ops wildcard DELETE expected 200");

    @print("PASS @web/rbac (4/4 tests)");
}

#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Exercise opt-in offline edits and local backups; requires a disconnected app."""

import json
import os
from pathlib import Path
import sys
import uuid
import urllib.request
import xml.etree.ElementTree as ET


def main():
    url = sys.argv[1]
    request_id = 0

    def send(method, params=None):
        nonlocal request_id
        request_id += 1
        data = {"jsonrpc": "2.0", "id": request_id, "method": method}
        if params is not None:
            data["params"] = params
        request = urllib.request.Request(url, json.dumps(data).encode(), headers={
            "Authorization": "Bearer " + os.environ["VESC_MCP_TOKEN"],
            "Content-Type": "application/json", "Accept": "application/json, text/event-stream",
            "MCP-Protocol-Version": "2025-11-25"})
        with urllib.request.urlopen(request, timeout=5) as response:
            reply = json.load(response)
        if reply.get("id") != request_id or "error" in reply:
            raise RuntimeError("RPC failed")
        return reply["result"]

    def tool(name, args=None, expect_error=False):
        reply = send("tools/call", {"name": name, "arguments": args or {}})
        if bool(reply.get("isError")) != expect_error:
            raise RuntimeError("Unexpected tool outcome: " + name)
        return json.loads(reply["content"][0]["text"])

    def require(condition, message):
        if not condition:
            raise RuntimeError(message)

    send("initialize", {"protocolVersion": "2025-11-25", "capabilities": {},
                        "clientInfo": {"name": "offline-edit-test", "version": "1"}})
    status = tool("vesc_get_status")
    require(not status["connected"], "Disconnect the controller before this test.")
    require(status["access"] == "app_edits", "Opt-in edit mode is required.")
    require(not status["controllerWritesEnabled"], "Controller writing must be disabled.")
    original = tool("vesc_get_config", {"config": "app"})
    old = original["values"]["controller_id"]
    new = old + 1 if old < 253 else old - 1
    args = {"config": "app", "changes": {"controller_id": new},
            "expectedSnapshot": original["snapshot"]}
    preview = tool("vesc_preview_config_edit", args)
    require(not preview["applied"], "Preview applied an edit.")
    require(tool("vesc_get_config", {"config": "app"})["snapshot"] == original["snapshot"],
            "Preview changed app memory.")
    invalid = dict(args, changes={"controller_id": new, "not_a_parameter": 1})
    tool("vesc_apply_config_edit", invalid, expect_error=True)
    require(tool("vesc_get_config", {"config": "app"})["snapshot"] == original["snapshot"],
            "Invalid batch changed app memory.")
    edited = tool("vesc_apply_config_edit", args)
    require(edited["applied"] and edited["verified"] and not edited["controllerWritten"],
            "Offline edit was not verified.")
    backup = json.loads(Path(edited["backupJsonPath"]).read_text(encoding="utf-8"))
    require(backup["values"]["controller_id"] == old, "Backup does not contain the original value.")
    require(tool("vesc_get_config", {"config": "app"})["values"]["controller_id"] == new,
            "Edit was not visible in app memory.")
    tool("vesc_apply_config_edit", args, expect_error=True)
    restored = tool("vesc_apply_config_edit", {
        "config": "app", "changes": {"controller_id": old}, "expectedSnapshot": edited["snapshot"]})
    require(restored["verified"], "Restoration failed.")
    require(tool("vesc_get_config", {"config": "app"})["snapshot"] == original["snapshot"],
            "App configuration did not return to its original state.")
    saved = tool("vesc_save_config_local", {"config": "app"})
    require(ET.parse(saved["xmlPath"]).getroot().tag == "APPConfiguration", "Invalid saved XML.")
    require(json.loads(Path(saved["jsonPath"]).read_text(encoding="utf-8"))["snapshot"] ==
            original["snapshot"], "Invalid saved JSON snapshot.")
    name = "mcp-test-" + uuid.uuid4().hex
    draft = tool("vesc_lisp_save", {"name": name, "source": "(print 1)"})
    require(tool("vesc_lisp_get", {"name": name})["sha256"] == draft["sha256"], "Draft read mismatch.")
    tool("vesc_lisp_save", {"name": name, "source": "(print 2)"}, expect_error=True)
    updated = tool("vesc_lisp_save", {"name": name, "source": "(print 2)", "expectedSha256": draft["sha256"]})
    require(updated["sha256"] != draft["sha256"], "Draft update failed.")
    tool("vesc_lisp_get", {"name": "../escape"}, expect_error=True)
    tool("vesc_lisp_save", {"name": "bad-import", "source": "(import \"file\" 'x)"}, expect_error=True)
    print("Offline preview, validation, edit, conflict rejection, restore and local XML/JSON save passed.")
    print("Original app-memory configuration restored. No controller writes requested.")
    print("Local LispBM draft create/read/conflict/update/path validation passed.")


if __name__ == "__main__":
    main()

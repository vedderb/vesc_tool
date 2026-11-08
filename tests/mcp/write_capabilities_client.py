#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Verify enabled upload tools fail closed in a DISCONNECTED native app."""

import json
import os
import sys
import urllib.request


def main():
    request_id = 0

    def rpc(method, params=None):
        nonlocal request_id
        request_id += 1
        message = {"jsonrpc": "2.0", "id": request_id, "method": method}
        if params is not None:
            message["params"] = params
        request = urllib.request.Request(sys.argv[1], json.dumps(message).encode(), headers={
            "Authorization": "Bearer " + os.environ["VESC_MCP_TOKEN"],
            "Content-Type": "application/json", "Accept": "application/json, text/event-stream",
            "MCP-Protocol-Version": "2025-11-25"})
        with urllib.request.urlopen(request, timeout=5) as response:
            reply = json.load(response)
        if "error" in reply:
            raise RuntimeError("RPC failed")
        return reply["result"]

    def call(name, args):
        return rpc("tools/call", {"name": name, "arguments": args})

    rpc("initialize", {"protocolVersion": "2025-11-25", "capabilities": {},
                       "clientInfo": {"name": "disconnected-write-test", "version": "1"}})
    status = json.loads(call("vesc_get_status", {})["content"][0]["text"])
    if status["connected"]:
        raise RuntimeError("This test must only run against a disconnected test app.")
    if not status["controllerWritesEnabled"]:
        raise RuntimeError("Start the test app with --mcpAllowControllerWrites.")
    tools = {tool["name"]: tool for tool in rpc("tools/list")["tools"]}
    for name in ("vesc_controller_upload", "vesc_lisp_upload"):
        if name not in tools or tools[name]["annotations"]["readOnlyHint"]:
            raise RuntimeError("Missing or incorrectly annotated upload tool")
    for kind in ("motor", "app"):
        current = json.loads(call("vesc_get_config", {"config": kind})["content"][0]["text"])
        for operation in ("vesc_controller_upload", "vesc_controller_download"):
            reply = call(operation, {"config": kind, "expectedSnapshot": current["snapshot"]})
            if not reply.get("isError"):
                raise RuntimeError("Disconnected controller operation was accepted.")
    for name in ("vesc_rt_capture", "vesc_lisp_download"):
        if not call(name, {}).get("isError"):
            raise RuntimeError("Disconnected operation was accepted: " + name)
    for name in ("CON", "../escape"):
        if not call("vesc_lisp_get", {"name": name}).get("isError"):
            raise RuntimeError("Unsafe draft name was accepted.")
    print("Upload tool discovery/annotations, disconnected transfers/capture and draft path guards passed.")
    print("No controller job or hardware write was started.")


if __name__ == "__main__":
    main()

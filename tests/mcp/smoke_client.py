#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only integration check against an explicitly started native app endpoint."""

import json
import os
import sys
import urllib.request


def main():
    url = sys.argv[1] if len(sys.argv) > 1 else "http://127.0.0.1:65125/mcp"
    token = os.environ["VESC_MCP_TOKEN"]
    request_id = 0

    def send(method, params=None, notification=False):
        nonlocal request_id
        request_id += 1
        message = {"jsonrpc": "2.0", "method": method}
        if not notification:
            message["id"] = request_id
        if params is not None:
            message["params"] = params
        request = urllib.request.Request(
            url, data=json.dumps(message).encode("utf-8"),
            headers={"Authorization": "Bearer " + token,
                     "Content-Type": "application/json",
                     "Accept": "application/json, text/event-stream",
                     "MCP-Protocol-Version": "2025-11-25"})
        with urllib.request.urlopen(request, timeout=5) as response:
            body = response.read()
            if notification:
                if response.status != 202 or body:
                    raise RuntimeError("Invalid notification response")
                return None
            reply = json.loads(body)
            if reply.get("id") != request_id or "error" in reply:
                raise RuntimeError("RPC request failed: " + method)
            return reply["result"]

    initialized = send("initialize", {
        "protocolVersion": "2025-11-25", "capabilities": {},
        "clientInfo": {"name": "vesc-tool-smoke-test", "version": "1"}})
    if initialized["protocolVersion"] != "2025-11-25":
        raise RuntimeError("Unexpected protocol version")
    send("notifications/initialized", notification=True)
    tools = send("tools/list")["tools"]
    names = {tool["name"] for tool in tools}
    if names != {"vesc_get_status", "vesc_get_config", "vesc_get_rt_data",
                 "vesc_controller_download", "vesc_get_job", "vesc_cancel_job", "vesc_rt_capture",
                 "vesc_lisp_download", "vesc_lisp_get"}:
        raise RuntimeError("Unexpected tool surface")
    for name, arguments in (
            ("vesc_get_status", {}),
            ("vesc_get_config", {"config": "motor"}),
            ("vesc_get_config", {"config": "app"}),
            ("vesc_get_rt_data", {})):
        result = send("tools/call", {"name": name, "arguments": arguments})
        if result.get("isError"):
            raise RuntimeError("Tool failed: " + name)
        data = json.loads(result["content"][0]["text"])
        if name == "vesc_get_config":
            if data["source"] != "app_memory" or data["controllerValuesVerified"]:
                raise RuntimeError("Incorrect configuration provenance")
            if not data["values"]:
                raise RuntimeError("App configuration was not loaded")
            print(name + " (" + arguments["config"] + "): " +
                  str(len(data["values"])) + " parameters")
        else:
            print(name + ": OK")
    for name, arguments in (
            ("vesc_get_config", {"config": "invalid"}),
            ("vesc_get_config", {"config": "motor", "names": ["not_a_real_parameter"]}),
            ("vesc_get_config", {"config": "app", "names": "invalid"}),
            ("vesc_get_config", {"config": "motor", "unexpected": True}),
            ("vesc_get_status", {"unexpected": True}),
            ("vesc_get_rt_data", {"unexpected": True})):
        invalid = send("tools/call", {"name": name, "arguments": arguments})
        if not invalid.get("isError"):
            raise RuntimeError("Invalid tool arguments were not rejected: " + name)
    send("ping")
    print("Native app MCP smoke test passed. No controller commands requested.")


if __name__ == "__main__":
    main()

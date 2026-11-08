#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Read-only interoperability/load check using the official MCP Python SDK 2.3."""

import argparse
import asyncio
import json
import os
from collections import Counter

import anyio
import httpx2
from mcp import Client
from mcp.client.streamable_http import streamable_http_client


async def check(url, iterations, clients, mode):
    counts = Counter()

    async def observe(response):
        # Never log headers, credentials, config values or the token.
        counts[(response.request.method, response.status_code)] += 1

    async def session(index):
        async with httpx2.AsyncClient(
                headers={"Authorization": "Bearer " + os.environ["VESC_MCP_TOKEN"]},
                trust_env=False, timeout=10,
                event_hooks={"response": [observe]}) as http:
            transport = streamable_http_client(url, http_client=http)
            async with Client(transport, read_timeout_seconds=10, mode=mode) as client:
                tools = await client.list_tools(meta={"progressToken": "sdk-test"})
                if {tool.name for tool in tools.tools} != {
                        "vesc_get_status", "vesc_get_config", "vesc_get_rt_data",
                        "vesc_controller_download", "vesc_get_job", "vesc_cancel_job", "vesc_rt_capture",
                        "vesc_lisp_download", "vesc_lisp_get"}:
                    raise RuntimeError("Unexpected tools")
                if index == 0:
                    print("Negotiated protocol:", client.protocol_version)
                    for name, args in (
                            ("vesc_get_config", {"config": "motor"}),
                            ("vesc_get_config", {"config": "app"}),
                            ("vesc_get_rt_data", {})):
                        reply = await client.call_tool(name, args)
                        if reply.is_error:
                            raise RuntimeError("Read failed: " + name)
                        json.loads(reply.content[0].text)
                for _ in range(iterations):
                    reply = await client.call_tool("vesc_get_status", {})
                    if reply.is_error:
                        raise RuntimeError("Status read failed")
                    data = json.loads(reply.content[0].text)
                    if data["access"] != "read_only":
                        raise RuntimeError("Unexpected access level")
                invalid = await client.call_tool("vesc_get_config", {"config": "invalid"})
                if not invalid.is_error:
                    raise RuntimeError("Invalid config arguments were accepted")
                return iterations

    with anyio.fail_after(90):
        results = await asyncio.gather(*(session(i) for i in range(clients)))
    print("Status calls:", sum(results), "across", clients, "SDK clients")
    print("HTTP responses:", dict(sorted(counts.items())))
    print("Official SDK check passed; no controller commands requested.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("url", nargs="?", default="http://127.0.0.1:65125/mcp")
    parser.add_argument("--iterations", type=int, default=100)
    parser.add_argument("--clients", type=int, default=1)
    parser.add_argument("--mode", choices=("auto", "legacy"), default="auto")
    args = parser.parse_args()
    if args.iterations < 1 or args.clients < 1 or args.clients > 4:
        parser.error("iterations must be positive and clients must be between 1 and 4")
    anyio.run(check, args.url, args.iterations, args.clients, args.mode)


if __name__ == "__main__":
    main()

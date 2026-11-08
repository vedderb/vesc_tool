# Native MCP access in VESC Tool

Experimental desktop integration for review and testing. This is a contribution
branch, not an official binary release. Persistent configuration and LispBM
operations are implemented, but **have not been tested on a physical controller**.

The server runs inside VESC Tool and uses its existing connection and parameter
models. It does not open a second controller connection or require a QML/Python
bridge. No extra runtime library is needed beyond the app's existing Qt modules.

## Start and permissions

Build the normal `vesc_tool.pro` with Qt 5 / qmake. Set `VESC_MCP_TOKEN` to a
random secret of 32–256 ASCII letters, digits, underscores or hyphens; use at
least 256 bits of randomness, for example `secrets.token_hex(32)` in Python.
Never commit the token or put it into command-line arguments.

Start the built executable with the following options (actual filename varies):

| Options | Access |
| --- | --- |
| `--mcpServer 65125` | App/config reads, controller downloads, RT capture, job status/cancellation and local draft reads. |
| Add `--mcpAllowEdits` | App-memory edits, local configuration backups and LispBM draft creation/update. |
| Add `--mcpAllowControllerWrites` | Also enables local edits and controller/config or LispBM upload requests. Each upload needs manual approval in VESC Tool. |

MCP is off during an ordinary launch. The desktop status bar shows the enabled
access level and recent tool calls. Closing that app stops the endpoint. There
is no persisted enablement or command for attaching to an already-running app.

Download tools do not persistently write the controller, but update the GUI's
parameter model and create local backup files. Uploads are not enabled by a
mere read token. The token permits all advertised tools for that app instance;
it is not a per-client role system.

## Connect an MCP client

- URL: `http://127.0.0.1:65125/mcp`
- Transport: Streamable HTTP with JSON responses; no sessions or SSE stream.
- Header: `Authorization: Bearer <VESC_MCP_TOKEN>`
- POST Content-Type: `application/json`
- Accept: `application/json, text/event-stream`
- Implemented protocol: `2025-11-25`; also negotiates the shared subset with
  `2025-03-26`. The newer 2026 discovery protocol is not implemented.
- Perform initialize / notifications/initialized normally.
- GET and DELETE return 405. Requests use Content-Length; no chunked request
  bodies, JSON-RPC batches, pipelining or server-initiated requests.

This local endpoint uses an explicitly supplied bearer secret, not an OAuth
authorization/discovery server. Do not proxy or expose it to a network. Only
IPv4 loopback is bound. Host must match localhost or 127.0.0.1 plus the port.
Every browser Origin is rejected. Requests are limited to 8 KiB of headers,
64 KiB of body, 8 concurrent connections and a 5-second deadline.

## Parameter and configuration tools

| Tool | Arguments / result |
| --- | --- |
| `vesc_get_status` | Connection, firmware/limited mode, selected CAN route, permissions and busy state. |
| `vesc_get_config` | `config: "motor" | "app"`; optional `names: string[]` up to 64. Returns app-memory values and a complete snapshot SHA-256. |
| `vesc_preview_config_edit` | `config`, `changes: object`, `expectedSnapshot`. Validate before/after values without changing anything. |
| `vesc_apply_config_edit` | Same. Back up and change app memory through normal ConfigParams setters. Does not send a configuration write. |
| `vesc_save_config_local` | `config`. Save standard XML and exact JSON snapshots. |
| `vesc_controller_download` | `config`, `expectedSnapshot`. Start fresh controller read, update the GUI model, save backups and return a job id. |
| `vesc_controller_upload` | `config`, `expectedSnapshot`. Upload the current proposed app configuration after fresh controller read, backup, GUI review and approval; verify ACK/readback. Returns a job id. |

Read a current snapshot before editing or starting a transfer. An outdated
snapshot rejects the request. Snapshot hashing covers the full app config and
serialization signature even when only selected values were returned.

App-memory results explicitly say `source: "app_memory"` and
`controllerValuesVerified: false`. Defaults and unsaved edits are included.
Only a successful fresh download job has controller provenance; its values are
the firmware-serialized parameter subset. Capture the app state you intend
to upload before requesting a download, which replaces the GUI model with
controller values. Every transfer backs up the prior app-memory state first.

Edits accept 1–64 known names. JSON types, numeric limits, enum indices, string
lengths and embedded NULs are checked. Units are raw ConfigParams units, not
editor-scaled percentages. Bitfield edits are unsupported. Existing unchanged
bitfields can still be included in full configuration transfers.

Local backups use unique names and QSaveFile in the per-user app data directory
under `mcp/backups` or `mcp/controller-backups`; callers cannot choose arbitrary
paths. XML uses the existing GUI import format. JSON/wire backups retain exact
precision; the existing XML serializer prints doubles with six digits.
If a required backup fails, no subsequent edit/upload occurs. An already-saved
companion file can remain if another backup file fails.

Parameter setters notify the normal GUI. Dependent handlers can alter other
parameters; edits validate first and verify requested values afterwards, but
are not an atomic transaction across GUI callbacks. Inspect the full result and
backup after an error rather than retrying an old snapshot.

## Controller jobs and approval

`vesc_get_job` takes `id` and optional `includeSamples: true`.
`vesc_cancel_job` takes `id`. Keep polling until state is succeeded or failed;
receiving a job id never means the device operation succeeded. The most recent
eight jobs are retained in memory.

An upload owns the app command channel and blocks normal Commands senders and
TCP/UDP forwarding. CAN route setters are locked. Existing TCP/UDP servers must
be stopped before starting a job. The VESC window presents progress/cancel and
a separate manual confirmation with device identity and changes/source.
The progress window's Stop motor / cancel button also sends the existing app
stop command. API cancellation alone does not send motor-control commands.

Writes currently require active input mode **No App, ADC or PPM**. ADC channels
or PPM must be neutral and freshly decoded. Unsupported combined/UART/etc.
modes fail closed. Telemetry/input freshness and standstill are checked before
the write and readback; flash pauses wait for fresh data within the operation
deadline. Changing a forwarded device's CAN id is refused because readback on
the old route would be unreliable; connect directly for that change.

Wire values are normalized before review to account for firmware encoding.
A matching ACK is followed by a new read and exact canonical comparison.
Connection/identity/schema changes, unsafe input, wrong ACK or mismatched
readback fail the job. There are no automatic write retries or rollback writes.
After any issued mutation, cancellation/failure reports `outcomeUnknown`:
it does not mean the hardware stayed unchanged.

These guards are app-side protection, not a certified motor safety function.
The VESC protocol does not correlate configuration ACKs with request UUIDs.
Other physical CAN/UART writers cannot be excluded by a local app lock.
Reviewing and isolating the actual controller setup remains necessary.

Older supported firmware, including 6.06, can leave the GUI in limited mode
even after VESC Tool loads its matching parameter schema. MCP accepts that
case only with a matching loaded schema and continues to enforce Commands'
per-command compatibility allowlist. Unsupported commands remain blocked;
MCP never clears limited mode or updates firmware. Status reports the detected
firmware version, hardware and whether the loaded configuration schema matches.

## RT recording and analysis

- `vesc_get_rt_data`: last already-received app packet, availability, age and
  stale flag. It does not start polling.
- `vesc_rt_capture`: `durationMs` 100–60000 (default 1000) and `intervalMs`
  50–1000 (default 50). Starts an asynchronous recording through the app.
- Read its job for per-field count, minimum, maximum, mean, RMS and population
  standard deviation. Add includeSamples to retrieve the bounded samples.

Samples use app receipt timestamps. Missing selective fields are omitted, not
filled with zeros or merged from old packets. Fresh ADC/PPM inputs are included
when available; voltage/current packets also provide derived input power.
ERPM is electrical speed, duty is a fraction, names otherwise carry units.
The low-speed-current count is an observation, not a diagnosis of a stalled
motor. RT capture sends read requests only, never motor-control commands.
Completed captures save samples and analysis as local JSON and return
`recordingPath`. A save failure is reported while samples remain in the bounded
job cache.

## LispBM drafts and transfer

| Tool | Arguments / effect |
| --- | --- |
| `vesc_lisp_save` | `name`, `source`; updating an existing file also requires `expectedSha256`. |
| `vesc_lisp_get` | `name`. Return local draft text, path and SHA-256. |
| `vesc_lisp_download` | No arguments. Start a controller-code download and save the exact binary payload locally. |
| `vesc_lisp_upload` | `name`, `expectedSha256`. Request upload of an immutable saved draft, with backup and manual GUI approval. |

Drafts live under `mcp/scripts` in app data. Names have 1–64 letters, digits,
underscores or hyphens; path traversal, reserved Windows device names and
symbolic-link drafts are refused. Sources must be nonempty, self-contained
UTF-8 of at most 32 KiB, without NUL or external imports. Draft saving does not
compile or execute LispBM. Existing imported code is preserved in binary
download/backup files, although new draft uploads do not pack external imports.

Uploads preserve the previous payload before mutation, use the firmware's
erase path to stop the VM, write CRC/header-compatible chunks and compare the
full downloaded payload with the proposed code. No auto-run request is sent.
The uploaded program can still run after a later controller restart.
The job records the source SHA-256. Failed or cancelled flash operations can
leave a partial/erased program; inspect the backup and device before retrying.
Controller-code downloads are bounded to 128 KiB.

## Tests and validation

Hardware-free QtTest targets (build each in its own external shadow directory):

| qmake project | Windows executable | Checks |
| --- | --- | --- |
| `tests/mcp/mcp.pro` | `release/tst_mcpserver.exe` | HTTP/RPC, token/Origin/Host, framing, limits, metadata and parameter validation. |
| `tests/mcp/transactions.pro` | `release/tst_mcpwritetransaction.exe` | Write states, approval, guards, cancellation, deadlines and readback. |
| `tests/mcp/deviceoperations.pro` | `release/tst_mcpdeviceoperations.exe` | Simulated controller config/RT/LispBM roundtrips, failures and flash pause. |
| `mcp_tests.pro` | `build/win/tst_mcpapplication.exe` with release_win | Actual Commands ownership, route lock, reentrant callers and owner lifetime. |

Use qmake, then make (mingw32-make on Windows), then run the target. Unix test
binaries normally live in the build root. The application-boundary target
needs the app's full Qt dependencies; the first three use Qt Core/Gui/Network/
Test only. The GitHub workflow runs those three on Ubuntu.

For a started, disconnected native app, with the same token in the environment:

```sh
python tests/mcp/smoke_client.py http://127.0.0.1:65125/mcp
# With --mcpAllowEdits, tests temporary app edits, restore and Lisp draft CRUD:
python tests/mcp/edit_client.py http://127.0.0.1:65125/mcp
# Optional official SDK interoperability/load test, in an isolated environment:
python -m pip install -r tests/mcp/requirements-sdk.txt
python tests/mcp/sdk_client.py http://127.0.0.1:65125/mcp --clients 4 --iterations 250
```

Use the normal Windows desktop platform for app-level tests. This installed
Qt 5.15.2 kit crashes in QMessageBox::showEvent / qt_getWindowsSystemMenu when
a startup message appears under --offscreen, also with MCP disabled.

Validation uses Windows / Qt 5.15.2 / MinGW 8.1, fake controller responses and
the official MCP Python SDK 2.3.0. SDK default discovery for a newer protocol
receives HTTP 400 and falls back to initialization with 2025-11-25; --mode legacy
tests the classic handshake directly. No physical controller write or LispBM
execution has been performed. Simulation cannot replace device/firmware testing.

See [controller-write implementation](CONTROLLER-WRITES.md) for the job boundary.
No personal paths, secrets, generated binaries or hardware backups belong in
this contribution. VESC Tool remains GPL-3.0-or-later.

References:
- [MCP transport](https://modelcontextprotocol.io/specification/2025-11-25/basic/transports)
- [MCP lifecycle](https://modelcontextprotocol.io/specification/2025-11-25/basic/lifecycle)
- [MCP tools](https://modelcontextprotocol.io/specification/2025-11-25/server/tools)
- [Firmware configuration and Lisp code packets](https://github.com/vedderb/bldc/blob/master/comm/commands.c)
- [Firmware flash code layout](https://github.com/vedderb/bldc/blob/master/flash_helper.c)

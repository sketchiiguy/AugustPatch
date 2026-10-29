# AugustPatch

Client patch for **Rec Room 2024, build 20240802.02 (August 9th 2024)**. It points the PC client at your own
server and switches off the checks that would otherwise close the game.


# Discord
https://discord.gg/rGwFmAKFPZ join for a good revival
Or DM sketchgaming123 for help

| | |
|---|---|
| Steam manifest | `-app 471710 -depot 471711 -manifest 7267876679868989367` |
| Platform | PC only |
| Output | `2024AugustPatch.dll` + `Injector.exe` |

The patch only works on this exact build. Method addresses are fixed to it, so any other build will fail to hook
or crash. `dump.cs` and `methods.csv` for the dump files where the addresses were taken from will be added to a drive soon.

## Setup

1. Open `src/core/settings.h` and replace the two placeholders:

   | Setting | Placeholder | What it is |
   |---|---|---|
   | `ApiHost` | `REPLACE_URL` | Your server's host name. It replaces `ns.rec.net` in every API request |
   | `PhotonHost` | `REPLACE_VPS_IP` | Your server's IP. Photon, ExitGames and PhotonIndustries lookups resolve to it |

   The rest of the file holds on/off switches (console, dead-host blocking, anti-cheat, tracing).
   Everything is compiled in. The DLL reads no config file.

2. Build:

   ```bat
   build.bat
   ```

   It does a full Release x64 rebuild and prints the SHA-256 of both outputs. The files land in `x64\Release\`.
   Other forms:

   ```bat
   build.bat Debug
   build.bat deploy
   ```

   `deploy` copies both files into `D:\Sketch\2024AugustPatch`. Set `PATCH_DEPLOY_DIR` to use a different folder.
   `build.bat` finds MSBuild itself, or you can set `MSBUILD_EXE`.

3. Copy `2024AugustPatch.dll` and `Injector.exe` into the game folder, next to `RecRoom.exe`.

## Running

```bat
Injector.exe
```

The injector starts the game, waits for `GameAssembly.dll` to load, and injects the DLL straight away. This build
closes itself about 9 seconds after launch if it can't reach rec.net, so the patch has to go in early.

| Option | Effect |
|---|---|
| `--dll <path>` | Use a DLL other than `2024AugustPatch.dll` next to the injector |
| `--exe <path>` | Launch a different executable (default: `RecRoom.exe`, then `Recroom_Release.exe`) |
| `--no-launch` | Inject into an already running game |
| `--no-wait` | Exit after injecting instead of staying open for the live log |

A live console shows the log. It is also written to `2024augustpatch.log`.

## What it patches

| Area | Effect |
|---|---|
| Requests | Rewrites `ns.rec.net` to `ApiHost` before the request is sent |
| DNS | `getaddrinfo` / `GetAddrInfoW` answer Photon hosts with `PhotonHost` and block dead hosts |
| TLS and cert pinning | Accepts your server's certificate |
| Anti-cheat | Silences `CheatManager` detectors and the DUID mismatch check, and answers the EAC challenge |
| Quit funnel | Stops the client closing itself (`FatalApplicationQuit`, `TryApplicationQuit`, `IsQuitting*`) |
| Launch validation | Neutralises the hash-check thread and the boot validation, and skips image signature verification |
| Photon | Keeps the client connected through disconnects and status changes |
| Cheat reports | Logs each `PlayerReporting.CheatReport` (reason, detail, caller) and passes it on unchanged |

Hooks are queued and switched on with a single `MH_ApplyQueued`, which takes about 60 ms. The log prints one
`[Patch] … hooked at GA+0x…` line per hook, and a `FAILED` or `SKIPPED` line if one didn't apply.

## Layout

```
src/
  entry/dllmain.cpp   DLL entry, console thread, install
  core/               console, signature scanning, runtime globals, settings
  game/addresses.h    method and field addresses for this build
  hooks/              one header per hook group; install.h wires them up
  vendor/             MinHook, spoofcall, string obfuscation
Injector/             launcher and injector
```

## If it stops working

- **Nothing happens, or the game closes:** the build is wrong. Check the log for `SKIPPED` or `FAILED` lines.
- **Requests still go to rec.net, or Photon never connects:** `ApiHost` or `PhotonHost` is still a placeholder. Fix `settings.h` and rebuild.
- **Hooks install but the game still quits:** it was injected too late. Use `Injector.exe` without `--no-launch`.

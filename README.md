# NoQuestPopup

A proxy DLL mod for **Oblivion Remastered** that suppresses all quest popup
notifications (Quest Added, Quest Updated, Quest Completed) at runtime.
The original game executable is never modified on disk.

---

## Table of Contents

- [Installation](#installation)
- [Uninstallation](#uninstallation)
- [Verifying It Works](#verifying-it-works)
- [Compatibility](#compatibility)
- [How It Works (Overview)](#how-it-works-overview)
- [How It Works (Technical Deep-Dive)](#how-it-works-technical-deep-dive)
  - [Why a Proxy DLL?](#why-a-proxy-dll)
  - [Why version.dll?](#why-versiondll)
  - [DLL Loading and Forwarding](#dll-loading-and-forwarding)
  - [The Quest Popup Code Path](#the-quest-popup-code-path)
  - [Pattern Scanning](#pattern-scanning)
  - [The JZ-to-JMP Patch](#the-jz-to-jmp-patch)
- [Project Structure](#project-structure)
- [Building from Source](#building-from-source)
- [Development History and Lessons Learned](#development-history-and-lessons-learned)
- [Troubleshooting](#troubleshooting)

---

## Installation

1. Copy `version.dll` into the game's executable directory:
   ```
   <Steam>\steamapps\common\Oblivion Remastered\OblivionRemastered\Binaries\Win64\
   ```
2. Launch the game normally.
3. All quest popup notifications are now suppressed.

That's it. One file, no configuration, no other dependencies.

## Uninstallation

1. Delete `version.dll` from:
   ```
   <Steam>\steamapps\common\Oblivion Remastered\OblivionRemastered\Binaries\Win64\
   ```
2. Optionally delete `NoQuestPopup.log` from the same directory.
3. Quest popups will return on the next launch.

The game executable is never modified, so there is nothing to restore.

## Verifying It Works

After launching the game at least once with the DLL installed, check the log file
at `Binaries\Win64\NoQuestPopup.log`. A successful run looks like this:

```
[2026-02-21 11:12:06] Loaded real version.dll from C:\WINDOWS\system32\version.dll
[2026-02-21 11:12:06] NoQuestPopup v1.0 - patch thread started
[2026-02-21 11:12:09] Game module: base=0x00007FF696D40000, size=0x9E1E000
[2026-02-21 11:12:09] Pattern found at 0x00007FF69D32FD7C, JZ instruction at 0x00007FF69D32FD86 (RVA: 0x65EFD86)
[2026-02-21 11:12:09] SUCCESS: Patched JZ -> JMP at 0x00007FF69D32FD86 (quest popups suppressed)
```

Key things to confirm:
- The real `version.dll` loaded from `C:\WINDOWS\system32\`
- The RVA of the JZ instruction is `0x65EFD86`
- The final line says `SUCCESS`

## Compatibility

- **Game version**: Built and tested against the Oblivion Remastered executable
  as of February 2026. If the game updates and moves or changes the quest
  notification handler, the pattern scan will fail safely (no patch applied,
  error logged). The game will still run normally with popups re-enabled.
- **ReShade**: Compatible. ReShade typically uses `dxgi.dll`, which does not
  conflict with our `version.dll`.
- **OBSE**: Compatible. OBSE uses its own loader mechanism.
- **Other proxy DLL mods**: Compatible as long as they don't also proxy
  `version.dll`. If another mod uses `version.dll`, one of the two must be
  converted to a different proxy target.

---

## How It Works (Overview)

```
Game starts
    |
    v
Game loads version.dll (our proxy) from its own directory
    |
    +--> Our DLL loads the REAL version.dll from C:\Windows\System32\
    |    and forwards all 17 API calls to it transparently
    |
    +--> Our DLL spawns a background thread that:
         1. Waits 3 seconds for the game to finish initializing
         2. Reads the game's PE headers to find the code region
         3. Scans all executable memory for a unique 22-byte signature
         4. Patches a single conditional jump (JZ) into an unconditional
            jump (JMP), causing the quest popup code to be skipped
         5. Logs the result to NoQuestPopup.log
```

The patch is applied **only in memory**. The game's `.exe` file on disk is never
touched. When the game exits, the patch disappears. The next launch repeats the
process.

---

## How It Works (Technical Deep-Dive)

### Why a Proxy DLL?

A proxy DLL (also called a DLL wrapper or DLL hijack) exploits the Windows DLL
search order. When a program calls `LoadLibrary("version.dll")` or links against
it at compile time, Windows looks in these locations in order:

1. The directory containing the executable  <-- **our DLL goes here**
2. The system directory (`C:\Windows\System32\`)
3. The Windows directory
4. The current directory
5. Directories in the PATH environment variable

By placing our `version.dll` in the game's directory, Windows loads ours first.
Our DLL then manually loads the real one from `System32` and forwards every
function call to it. The game cannot tell the difference — it gets the exact same
API behavior — but our DLL also runs its own code on load.

This approach has major advantages over direct exe patching:
- **No file modification**: The original exe stays untouched. Steam file
  verification won't flag anything. Antivirus is less likely to complain.
- **Easy install/uninstall**: Drop one file in / delete one file out.
- **Safe on game updates**: If the game updates and our pattern no longer
  matches, the patch simply doesn't apply. The game runs normally.
- **ASLR-compatible**: Since we scan for patterns at runtime, Address Space
  Layout Randomization doesn't matter.

### Why version.dll?

The choice of which system DLL to proxy is critical. The game must actually
import the DLL, or Windows will never load our proxy.

We inspected the game's import table with `dumpbin /IMPORTS` and found:
```
VERSION.dll      <-- chosen: small, simple, universally loaded
XINPUT1_3.dll
dxgi.dll         <-- taken by ReShade
DSOUND.dll
WINMM.dll
KERNEL32.dll
USER32.dll
... (and others)
```

`version.dll` was chosen because:
- The game **imports it directly** (calls `GetFileVersionInfoA`,
  `GetFileVersionInfoSizeA`, `VerQueryValueA`)
- It has only **17 exports** — manageable to wrap by hand
- It's **not used by ReShade or OBSE**, avoiding conflicts
- It's a classic, proven proxy DLL target in the modding community

An initial attempt used `dinput8.dll`, which is a popular proxy target for older
games. However, Oblivion Remastered (being a UE5 game) does not use DirectInput
at all — it uses XInput for controllers and raw input for keyboard/mouse.
The `dinput8.dll` proxy was never loaded by the game.

### DLL Loading and Forwarding

When `DllMain` is called with `DLL_PROCESS_ATTACH`, our DLL:

1. **Loads the real `version.dll`** from `C:\Windows\System32\version.dll`
   using an explicit absolute path (to avoid loading ourselves recursively).

2. **Resolves all 17 function pointers** via `GetProcAddress`:
   ```
   GetFileVersionInfoA         GetFileVersionInfoW
   GetFileVersionInfoExA       GetFileVersionInfoExW
   GetFileVersionInfoSizeA     GetFileVersionInfoSizeW
   GetFileVersionInfoSizeExA   GetFileVersionInfoSizeExW
   GetFileVersionInfoByHandle
   VerFindFileA                VerFindFileW
   VerInstallFileA             VerInstallFileW
   VerLanguageNameA            VerLanguageNameW
   VerQueryValueA              VerQueryValueW
   ```

3. **Exports wrapper functions** that simply call through to the real DLL.
   The `.def` file maps the canonical export names to our `proxy_*` functions:
   ```
   GetFileVersionInfoA = proxy_GetFileVersionInfoA
   VerQueryValueA      = proxy_VerQueryValueA
   ... etc
   ```
   The game calls `GetFileVersionInfoA` -> our `proxy_GetFileVersionInfoA`
   runs -> which calls the real `GetFileVersionInfoA` in the system DLL.

4. **Spawns a background thread** to perform the memory patch.

### The Quest Popup Code Path

Through reverse engineering with Ghidra, the following call chain was identified:

```
TESQuest_SetStage          (VA 0x1466ec2d0)
    |
    v
TESQuest_SetStageDone      (VA 0x1466e9120)
    |
    v
Quest notification handler (VA 0x1465efcb0)   <-- patch target is in here
    |
    v
Menu open function         (VA 0x146568990)
    opens "quest_added.xml" as a UE5 widget
```

When a quest stage is set (quest added, updated, or completed), the game calls
through this chain. The notification handler at `0x1465efcb0` contains a
conditional branch that checks whether to show the popup. The relevant assembly:

```asm
; ... earlier code stores result in RBX ...
48 89 03              MOV  [RBX], RAX          ; store quest data
E8 AC F3 03 00        CALL FUN_14662f130       ; check if popup should show
84 C0                 TEST AL, AL              ; test return value
0F 84 F4 09 00 00     JZ   skip_popup_code     ; if zero, skip popup <-- TARGET
49 8B D5              MOV  RDX, R13            ; (popup setup continues...)
49 8B CF              MOV  RCX, R15
E8 19 E4 00 00        CALL another_function
```

The `JZ` (Jump if Zero) at this location controls whether the quest popup UI is
displayed. When the function `FUN_14662f130` returns `AL=0` (false), the popup
code is skipped. When `AL=1` (true), execution falls through to the popup code.

**Our patch changes the `JZ` to a `JMP`** — an unconditional jump that always
skips the popup code, regardless of the return value. All three notification
types (Added, Updated, Completed) flow through this same handler, so one patch
suppresses all of them.

### Pattern Scanning

Rather than patching at a hardcoded address (which would break with ASLR or game
updates), we scan the game's code section for the unique byte pattern surrounding
the patch target. The full 22-byte signature:

```
48 89 03                    MOV [RBX], RAX      (3 bytes, context before)
E8 ?? ?? ?? ??              CALL ????????        (5 bytes, operand is wildcard)
84 C0                       TEST AL, AL          (2 bytes)
0F 84 ?? ?? ?? ??           JZ ????????          (6 bytes, operand is wildcard)
49 8B D5                    MOV RDX, R13         (3 bytes)
49 8B CF                    MOV RCX, R15         (3 bytes, context after)
```

The `??` bytes are wildcards — the CALL and JZ operands are relative offsets
that change depending on where the code is loaded, so we match only the opcodes.

**Why 22 bytes?** An earlier 16-byte pattern (without the `48 89 03` prefix and
`49 8B CF` suffix) matched **multiple locations** in the ~160 MB executable. The
scanner found the wrong match first (at RVA `0x2A9AB6A` instead of the correct
`0x65EFD86`) and patched an unrelated function. Adding 3 bytes of context on
each side made the signature unique — verified to produce exactly 1 match.

The scanner works by:
1. Getting the game's base address via `GetModuleHandleA(NULL)`
2. Reading the PE optional header to find `SizeOfImage`
3. Linear scanning from base to base+size, comparing each position against the
   pattern (skipping wildcard bytes)
4. Returning the first (and only) match

### The JZ-to-JMP Patch

The x86-64 instructions being modified:

| Instruction | Encoding | Size |
|---|---|---|
| `JZ rel32` (conditional) | `0F 84` + 4-byte signed offset | 6 bytes |
| `JMP rel32` (unconditional) | `E9` + 4-byte signed offset | 5 bytes |

Since `JMP rel32` is 5 bytes but `JZ rel32` is 6 bytes, and both encode a
relative offset from the **end** of the instruction, the math works out as:

- `JZ` offset is relative to `address_of_JZ + 6`
- `JMP` offset must be relative to `address_of_JZ + 5` (one byte earlier)
- To reach the same target: `new_offset = original_offset + 1`
- The 6th byte is replaced with `NOP` (`0x90`)

Before (original):
```
0F 84 F4 09 00 00     JZ +0x9F4 (relative to instruction end)
```

After (patched):
```
E9 F5 09 00 00        JMP +0x9F5 (relative to instruction end, same target)
90                    NOP (fills the remaining byte)
```

The patch uses `VirtualProtect` to temporarily change the memory page to
`PAGE_EXECUTE_READWRITE`, writes the 6 bytes, then restores the original
page protection.

---

## Project Structure

```
NoQuestPopup/
  build.bat            Build script (MSVC command line)
  version.dll          Compiled output (ready to install)
  version.lib          Import library (build artifact)
  version.exp          Export file (build artifact)
  dllmain.obj          Object file (build artifact)
  src/
    dllmain.c          Single source file (proxy + patcher, ~330 lines)
    version.def        Linker module definition (17 export mappings)
```

### src/dllmain.c

The entire mod is a single C source file, organized into four sections:

1. **Logging** (~20 lines) — Timestamped append to `NoQuestPopup.log`
2. **Proxy forwarding** (~130 lines) — Typedefs, function pointers, 17 wrapper
   functions, and the `load_real_version()` loader
3. **Pattern scanner + patcher** (~90 lines) — Signature definition,
   `find_pattern()` scanner, `apply_patch()` writer, and the `patch_thread()`
   orchestrator
4. **DLL entry point** (~30 lines) — `DllMain` handling attach/detach

### src/version.def

The module definition file tells the MSVC linker which functions to export and
how to name them. Each line maps a public export name (what the game expects) to
our internal `proxy_*` function:

```def
LIBRARY version
EXPORTS
    GetFileVersionInfoA = proxy_GetFileVersionInfoA
    VerQueryValueA      = proxy_VerQueryValueA
    ... (17 total)
```

### build.bat

A standalone build script that sets up MSVC and Windows SDK paths without
requiring a full Visual Studio Developer Command Prompt. Compiles with `/O2`
(optimize for speed) and `/LD` (create DLL).

---

## Building from Source

### Prerequisites

- **MSVC C/C++ compiler** (cl.exe) — from Visual Studio or Build Tools
- **Windows SDK** (10.0.26100.0 or similar) — for Windows headers and libs

### Steps

1. Open a command prompt (regular `cmd.exe` is fine, not PowerShell)
2. Navigate to the `NoQuestPopup` directory
3. Edit `build.bat` if your MSVC or SDK paths differ from the defaults
4. Run:
   ```
   build.bat
   ```
5. On success, `version.dll` appears in the current directory

The build script sets `PATH`, `INCLUDE`, and `LIB` environment variables
directly, so no prior environment setup (like `vcvarsall.bat`) is needed.

### Compile command breakdown

```
cl.exe /O2 /LD /Fe:version.dll src\dllmain.c /link /DEF:src\version.def kernel32.lib user32.lib
```

| Flag | Meaning |
|---|---|
| `/O2` | Optimize for speed |
| `/LD` | Create a DLL (sets entry point, links CRT as DLL) |
| `/Fe:version.dll` | Output filename |
| `/link` | Pass remaining args to the linker |
| `/DEF:src\version.def` | Module definition file for exports |
| `kernel32.lib` | Windows kernel APIs (VirtualProtect, LoadLibrary, etc.) |
| `user32.lib` | Windows user APIs |

---

## Development History and Lessons Learned

This mod went through several iterations:

### Attempt 1: XML modification
Modified `quest_added.xml` to hide the popup UI elements. **Did not work** —
Oblivion Remastered uses UE5 Slate/UMG widgets, not the XML-based UI from
the original game. The XML files exist for data but the actual rendering is
handled by native C++ widget code.

### Attempt 2: Direct exe binary patch
Used Ghidra to reverse-engineer the quest notification handler and found the
conditional jump controlling popup display. Patched the exe directly on disk
(changing bytes at file offset `0x65EF386`). **Worked perfectly**, but had
drawbacks: Steam would flag the modified exe, updates would overwrite it, and
distributing a patched exe is legally problematic.

### Attempt 3: Proxy DLL (dinput8.dll)
Created a `dinput8.dll` proxy to apply the same patch in memory at runtime.
**DLL was never loaded** — Oblivion Remastered (UE5) does not use DirectInput8.
It uses XInput for controllers. The import table showed no `dinput8.dll` entry.
Lesson: always verify the target actually imports the DLL you're proxying.

### Attempt 4: Proxy DLL (version.dll) with short pattern
Switched to `version.dll` (confirmed in the import table). DLL loaded and
reported a successful patch, but **popups still appeared**. The 16-byte pattern
had multiple matches in the 160 MB executable. The scanner found an unrelated
match at RVA `0x2A9AB6A` before reaching the correct one at `0x65EFD86`.
Lesson: always verify pattern uniqueness across the entire binary.

### Attempt 5: Proxy DLL (version.dll) with extended pattern
Extended the signature from 16 to 22 bytes by adding context instructions before
and after the target. Verified exactly 1 match in the entire executable.
**Works correctly** — quest popups are suppressed, game runs normally, no exe
modification on disk.

---

## Troubleshooting

### "Pattern not found" in log
The game was updated and the quest notification handler code changed. The byte
pattern no longer exists at any location. The mod needs to be updated with a new
signature. The game will run normally with popups re-enabled.

### No log file at all
The DLL is not being loaded. Possible causes:
- `version.dll` is not in the correct directory (`Binaries\Win64\`)
- Another mod is also proxying `version.dll` and loading first
- An antivirus or security tool is blocking DLL sideloading

### Log shows success but popups still appear
The pattern matched the wrong location (should not happen with the current
22-byte signature, but could occur after a game update introduces a new false
match). Check the RVA in the log — it should be `0x65EFD86`. If it's different,
the signature needs to be updated.

### Game crashes on startup
If the real `version.dll` fails to load from System32, `DllMain` returns FALSE
and the game will not start. Check the log for the error message. This could
indicate a corrupted Windows installation or aggressive security software
blocking `LoadLibraryA`.

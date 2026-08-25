Luvvy KH2 Mod Menu v0.5.0
=========================

Target
------
KINGDOM HEARTS II FINAL MIX.exe
Steam KINGDOM HEARTS HD 1.5+2.5 ReMIX
Steam 1.0.0.2, 64 bit

Core rule
---------
The v0.1.2 injection, overlay, and F10 path was confirmed working and remains the frozen core.
v0.5.0 keeps that core frozen and adds a separate CE instruction-hook layer for One Hit Kill and Free Command, plus a verified live-field Instant Haste module. The proxy, injection route, F10 toggle path, Square flag layer, and launcher binary were not changed.

Controls
--------
F10          Show or hide the menu
F9           Switch between Main and Advanced pages
F6 / F7      Previous or next visible item
F8           Toggle or run selected item
Up / Down    Alternate selection controls
Enter        Alternate toggle or run control
END          Restore reversible changes and unload KH2ModMenu.dll

The menu now contains 54 total actions or toggles, split into curated Main and Advanced pages.

Core cheats
-----------
* Infinite HP
* Infinite MP
* Infinite Drive Gauge
* Full Refill Now
* 0.5x Game Speed
* 2x Game Speed
* 4x Game Speed
* Lock Munny at 999999
* Max Drive Gauge 9 TEMP
* Unlock Standard Drive Forms TEMP EXP
* Max Form Levels 7 TEMP EXP
* Freeze Total Game Time CT
* Infinite Drive Form Duration CE
* EXP Gain Multiplier 2x / 4x / 8x CE
* Munny Gain Multiplier 2x / 4x / 8x CE
* Luvvy Turbo Movement CE
* Fly Mode with camera-relative WASD movement
* Low Gravity
* Freeze All Minigame Timers CE
* One Hit Kill CE HOOK
* Instant Haste, 100x MP recharge CE
* Free Command, commands never greyed CE PATCH
* Disable + Restore All

Freeze Total Game Time comes from the uploaded current Steam cheat table's direct Game Time address at executable RVA 0x009ABCF4. It freezes the total play-time counter at the value captured when enabled.

Main / Advanced pages
---------------------
F9 switches pages while the menu is open.

Main keeps the useful Square developer cheats and general gameplay cheats visible. Obscure diagnostic flags, the four red DANGER flags, temporary progression edits, and situational minigame tools are moved to Advanced.

Advanced includes the hidden Square developer switches plus:
* Unlock Standard Drive Forms TEMP EXP
* Max Form Levels 7 TEMP EXP
* Freeze Total Game Time
* Win Struggle Orbs Now
* Grandstander Combo 999 Now
* Activate All Existing Ability Slots

CE-derived fun modules
----------------------
The current Steam CE table supplied with this project was used for the pointer layouts and behavior of these isolated modules:

* Infinite Drive Form Duration continuously refreshes the active form timer from its live maximum.
* EXP Gain Multiplier cycles 1x -> 2x -> 4x -> 8x -> 1x and multiplies newly earned EXP. Already-added EXP is persistent.
* Munny Gain Multiplier cycles 1x -> 2x -> 4x -> 8x -> 1x and multiplies newly earned Munny. Already-added Munny is persistent.
* Luvvy Turbo Movement uses the CE table's per-form movement values and reacquires the current runtime movement pointer. When disabled, it restores the current form's values from the game's live base movement table.
* Fly Mode uses the CE player and camera pointer chains. With the menu closed: WASD moves camera-relative, Space moves up, Ctrl moves down, and Shift increases movement speed. Gravity is suppressed while flying.
* Low Gravity temporarily reduces the live player weight value and restores the captured value when disabled.
* Freeze All Minigame Timers continuously zeros the two timer globals identified by the CE table.
* Win Struggle Orbs Now writes 200 player orbs and 0 foe orbs. It is hidden in Advanced because it is minigame-specific.
* Grandstander Combo 999 Now writes the live and maximum combo counters to 999. It is hidden in Advanced because it is minigame-specific.
* Activate All Existing Ability Slots sets the activation bit on the ability slots listed by the CE table and captures those bytes for restoration. It does not grant missing ability IDs.

v0.5.0 adds three isolated CE-derived combat/command modules:

* One Hit Kill follows Gear2's defense hook. It verifies the original six instruction bytes at KH2 RVA 0x003C0870 before enabling, allocates a small executable cave within rel32 range of the game image, and redirects only that six-byte site. The player branch executes the original add/compare instructions. The enemy branch reproduces the table's forced lower-bound result. Disabling restores the exact original six bytes only if the site still contains this mod's own jump.
* Instant Haste verifies the Gear2 haste instruction at RVA 0x003D6986 and then uses the same live player haste field at +0x20C with value 100.0. This avoids installing a second code cave while reproducing the table's 100x MP recharge behavior. The captured field value is restored when possible.
* Free Command verifies the Gear2 command site at RVA 0x00402163. Instead of allocating a cave, it performs an equivalent eight-byte instruction reorder that zeros EAX before the command-disabled byte is written, while preserving the following mov rcx,rbx behavior. Disabling restores the exact original eight bytes only if our patch is still present.

Every code patch is blocked if the expected original instructions do not match at runtime. This also avoids overwriting another trainer or mod that has already changed the same site.

Square internal TEST_FLAG menu
------------------------------
The exact Steam executable was reverse inspected far enough to verify the actual internal state mapping, rather than only finding debug strings.

The game's YS::TEST_FLAG constructor:
* creates exactly 28 toggle entries
* indexes them from 0 through 27
* reads the bits sequentially from KINGDOM HEARTS II FINAL MIX.exe + 0x00749804
* writes them back through the game's own sequential bit setter

v0.5.0 still exposes those same 28 bits, but F9 hides the obscure and dangerous ones on the Advanced page:

00  Anytime Drive
01  Anytime Magic
02  R2 Map Debug
03  Skip Title
04  Ignore Zone
05  Test Limit
06  Exception  DANGER
07  GM Save Local
08  Stop Enemy
09  Demo Movie
10  Infinity Item
11  Copyright
12  Oni/Oshi
13  Clear Cache  DANGER
14  No Gameover
15  Anytime Mickey
16  Free Ability
17  Gentle Friend
18  Old Event Skip
19  Sound Log
20  No Check Effect Memory
21  GM Item Max
22  Memory Check  DANGER
23  No Pack Read  DANGER
24  Snapshot X2
25  Show Version
26  Caption Off
27  Words Line On

The mapping is verified from the supplied Steam EXE. The behavior of every developer flag has NOT been runtime tested. Several are developer diagnostics rather than normal cheats. Main keeps Anytime Drive, Anytime Magic, Skip Title, Stop Enemy, Infinity Item, No Gameover, Anytime Mickey, Free Ability, and GM Item Max visible. The rest are under Advanced.

DANGER flags
------------
Exception, Clear Cache, Memory Check, and No Pack Read are intentionally marked DANGER in red.
They are real Square developer flags, but their names indicate behavior that can intentionally throw an exception, invalidate runtime state, perform expensive memory diagnostics, or interfere with asset reads.
Do not test those on an unsaved session you care about.

Reversibility
-------------
The original Square TEST_FLAG DWORD is captured before the first internal flag is changed.
Disable + Restore All and END restore that original DWORD.
Existing reversible speed, Munny lock, Drive, form unlock, form-level, low-gravity, movement, and ability-slot captures are also restored when possible. Runtime pointers are reacquired rather than assumed to remain stable across rooms.

EXP and Munny gain multipliers intentionally change earned progression and therefore cannot undo gains already written to the save. A game save made while a progression or inventory-affecting debug option is active can also persist resulting game state. Restoring the flag itself cannot undo a save that already recorded an effect.

Build protection
----------------
Cheat writes are blocked unless the running executable matches the inspected Steam 1.0.0.2 layout:
* PE timestamp 0x669E384A
* SizeOfImage 0x02C2B000
* KH2J marker at RVA 0x009A98B0

Exact inspected executable:
SHA256 9002b2de6a1f91a790bd0673de125d1cf833f7942bfec827cdcf6ba64d5849ed

Uploaded cheat-table review
---------------------------
The newer KINGDOM HEARTS II FINAL MIX.CT targets the current PC executable and contains AOB or pointer implementations for additional features including EXP and Munny multipliers, one-hit-kill related damage logic, Instant Haste, Infinite Drive Form, Infinite Drive Meter, Gummi multipliers, minigame timer control, Struggle orb control, combo helpers, Teleport Fly, and Free Command.

v0.5.0 keeps the pointer/data modules from v0.4.0 and adds only the three requested CE features above. The remaining code-injection scripts are still not blindly transplanted. Each future hook should remain its own reversible module with an exact runtime byte check.

The older Kingdom Hearts II.CT contains many fixed-address save and inventory edits from a different memory layout. Its raw addresses are not copied into the Steam DLL.

Launcher status
---------------
The current Luvvy-KH2-Launch.exe is unchanged from v0.2.0.
On the tested Steam setup it still routes through the Square Enix collection selection menu, so it should NOT be considered a working collection-menu bypass yet.
This does not affect mod-menu injection or F10 operation.

Install
-------
1. Fully close KH2.
2. Open:
   C:\Program Files (x86)\Steam\steamapps\common\KINGDOM HEARTS -HD 1.5+2.5 ReMIX-
3. Replace KH2ModMenu.dll with the v0.5.0 copy.
4. dbghelp.dll is included for completeness and is byte-for-byte the same confirmed-working proxy from v0.2.0.
5. Keep Steam running and launch the game the same way that already worked for v0.2.0.
6. Select KH2 if Steam shows the collection menu.
7. Press F10.

Expected menu status
--------------------
Offsets: VERIFIED Steam 1.0.0.2

If it shows BLOCKED, no cheat writes occur.

Uninstall
---------
Close KH2 and remove the Luvvy files if desired.
No original Kingdom Hearts executable is replaced or patched on disk.

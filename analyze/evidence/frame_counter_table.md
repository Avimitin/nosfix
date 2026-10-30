| # | Site | Function | Op | Class | Current | Replacement rule |
|---:|---|---|---|---|---|---|
| 1 | `0x1801bc39f` | `unnamed`<br>chart marker advance (unnamed function 0x1801BC370) | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 2 | `0x1802fc55a` | `unnamed`<br>timeline reset A (unnamed 0x1802FC510) | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 3 | `0x1802fc5fe` | `unnamed`<br>timeline reset B returns counter pointer (unnamed 0x1802FC5B0) | `lea` | ADDR | `lea     rax, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 4 | `0x1802fc629` | `unnamed`<br>timeline reset B (unnamed 0x1802FC5B0) | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 5 | `0x180304dbb` | `unnamed`<br>remaining-time helper (unnamed 0x180304D90) | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 6 | `0x1801963a3` | `sub_180196390`<br>chart event dispatch / combined time | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 7 | `0x1801963f8` | `sub_180196390`<br>chart event dispatch / combined time | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 8 | `0x18019711d` | `sub_180197080`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 9 | `0x180197217` | `sub_1801971B0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 10 | `0x1801972c1` | `sub_1801972A0`<br>backtrack trigger at chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 11 | `0x18019735e` | `sub_180197330`<br>gameplay update / cue + timeline tick | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 12 | `0x18019744d` | `sub_180197330`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 13 | `0x180197527` | `sub_180197330`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 14 | `0x1801975e4` | `sub_180197330`<br>gameplay update / cue + timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 15 | `0x1801978ef` | `sub_180197770`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 16 | `0x180197f26` | `sub_180197C30`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 17 | `0x1801a58cc` | `sub_1801A58C0`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 18 | `0x1801a59d1` | `sub_1801A5990`<br>note judge window | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 19 | `0x1801a5a07` | `sub_1801A5990`<br>note judge window | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 20 | `0x1801a5e7c` | `sub_1801A5DB0`<br>note animation delta (unreachable block) | `sub` | DELTA | `sub     r10d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 21 | `0x1801a60e0` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 22 | `0x1801a61e1` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 23 | `0x1801a6522` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 24 | `0x1801a6624` | `sub_1801A60C0`<br>note judge window | `sub` | DELTA | `sub     r14d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 25 | `0x1801a68e1` | `sub_1801A6820`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 26 | `0x1801a691b` | `sub_1801A6820`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 27 | `0x1801a6d42` | `sub_1801A6820`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 28 | `0x1801a707e` | `sub_1801A6820`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 29 | `0x1801a74f8` | `sub_1801A7450`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 30 | `0x1801a7b70` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 31 | `0x1801a7d01` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     ebx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 32 | `0x1801a7d8a` | `sub_1801A7AF0`<br>note judge / long note | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 33 | `0x1801a849f` | `sub_1801A8490`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 34 | `0x1801a85ca` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 35 | `0x1801a8699` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     edx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 36 | `0x1801a8bde` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 37 | `0x1801a90b8` | `sub_1801A84F0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 38 | `0x1801a9b81` | `sub_1801A9AB0`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 39 | `0x1801aa38c` | `sub_1801AA340`<br>note state transition | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 40 | `0x1801aa43e` | `sub_1801AA340`<br>note state transition | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 41 | `0x1801aa545` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 42 | `0x1801aa597` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 43 | `0x1801aa9aa` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 44 | `0x1801aadc8` | `sub_1801AA4D0`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 45 | `0x1801aaf5b` | `sub_1801AA4D0`<br>note judge / animation | `mov` | READ | `mov     ecx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 46 | `0x1801ab508` | `sub_1801AB410`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 47 | `0x1801ac116` | `sub_1801ABFA0`<br>combined time / chart end | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 48 | `0x1801ac3dd` | `sub_1801ABFA0`<br>combined time / chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 49 | `0x1801ad44d` | `sub_1801AD3B0`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 50 | `0x1801ad547` | `sub_1801AD4E0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 51 | `0x1801ad5f1` | `sub_1801AD5D0`<br>backtrack trigger at chart end | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 52 | `0x1801ad68e` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `cmp` | CMP | `cmp     cs:dword_18070A610, edx` | `song_time_us >= event_us` (while-loop for crossings) |
| 53 | `0x1801ad76d` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 54 | `0x1801ad847` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 55 | `0x1801ad97a` | `sub_1801AD660`<br>gameplay update / cue + timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 56 | `0x1801adc47` | `sub_1801ADB00`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 57 | `0x1801ae23d` | `sub_1801ADFB0`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 58 | `0x1801ae7a4` | `sub_1801AE480`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 59 | `0x1801b8bf6` | `sub_1801B8660`<br>remaining-time readout | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 60 | `0x1801bc2ee` | `sub_1801BC240`<br>chart marker advance | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 61 | `0x1801bc45e` | `sub_1801BC3F0`<br>chart marker advance | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 62 | `0x1801bc5a7` | `sub_1801BC520`<br>chart section progress / marker | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 63 | `0x1801bc647` | `sub_1801BC520`<br>chart section progress / marker | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 64 | `0x1801bd176` | `sub_1801BD160`<br>progress bar ratio | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 65 | `0x1801c4787` | `sub_1801C3FE0`<br>music position in ms / audio sync | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 66 | `0x18021ad30` | `sub_18021ACF0`<br>combined time scene gate | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 67 | `0x1802f42b5` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     ebp, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 68 | `0x1802f451e` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     ebx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 69 | `0x1802f45ae` | `sub_1802F4220`<br>note judge window | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 70 | `0x1802f4e2f` | `sub_1802F4E20`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 71 | `0x1802f4f58` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 72 | `0x1802f500a` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     edx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 73 | `0x1802f54ef` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 74 | `0x1802f598c` | `sub_1802F4E80`<br>note judge / animation | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 75 | `0x1802f625e` | `sub_1802F6190`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 76 | `0x1802f6822` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 77 | `0x1802f6902` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 78 | `0x1802f6ace` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 79 | `0x1802f6c6c` | `sub_1802F6800`<br>note judge window | `sub` | DELTA | `sub     edi, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 80 | `0x1802f6ec7` | `sub_1802F6DF0`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 81 | `0x1802f6f1a` | `sub_1802F6DF0`<br>note judge / animation | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 82 | `0x1802f7456` | `sub_1802F73B0`<br>note judge window | `sub` | DELTA | `sub     r8d, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 83 | `0x1802f7903` | `sub_1802F7900`<br>note judge predicate | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 84 | `0x1802f8886` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     r15d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 85 | `0x1802f959a` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 86 | `0x1802f9a11` | `sub_1802F7C10`<br>judge / scoring combined time | `mov` | READ | `mov     r12d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 87 | `0x1802fc960` | `sub_1802FC960`<br>timeline counter accessor | `lea` | ADDR | `lea     rax, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 88 | `0x1802fdb9e` | `sub_1802FDB30`<br>gameplay controller / end check | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 89 | `0x1802fdcc3` | `sub_1802FDB30`<br>gameplay controller / end check | `mov` | READ | `mov     r8d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 90 | `0x1802fe00e` | `sub_1802FDB30`<br>gameplay controller / end check | `cmp` | CMP | `cmp     cs:dword_18070A610, ecx` | `song_time_us >= event_us` (while-loop for crossings) |
| 91 | `0x1802fec3d` | `sub_1802FEBA0`<br>sound cue scheduling | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 92 | `0x1802fed37` | `sub_1802FECD0`<br>sound cue scheduling / angle | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 93 | `0x1802fee0e` | `sub_1802FEDC0`<br>backtrack trigger at chart end | `mov` | READ | `mov     r15d, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 94 | `0x1802ff13d` | `sub_1802FF090`<br>gameplay update / timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 95 | `0x1802ff207` | `sub_1802FF090`<br>gameplay update / timeline tick | `sub` | DELTA | `sub     eax, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 96 | `0x1802ff2d9` | `sub_1802FF090`<br>gameplay update / timeline tick | `add` | ADD | `add     ecx, cs:dword_18070A610` | `song_time_us + offset_us` |
| 97 | `0x1802ff2f5` | `sub_1802FF090`<br>gameplay update / timeline tick | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 98 | `0x1802ff322` | `sub_1802FF090`<br>gameplay update / timeline tick | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 99 | `0x1802ff32e` | `sub_1802FF090`<br>gameplay update / timeline tick | `mov` | READ | `mov     edx, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 100 | `0x1802ffb44` | `sub_1802FF8D0`<br>timeline init from chart | `lea` | ADDR | `lea     rcx, dword_18070A610` | `&song_time_us`; delete 60-unit pointer |
| 101 | `0x18030038f` | `sub_1803001F0`<br>timeline reset | `mov` | RESET | `mov     cs:dword_18070A610, 0FFFFFF38h` | `-3,333,333 us` (truncated from -200/60 s) |
| 102 | `0x180304bf0` | `sub_180304BF0`<br>timeline counter accessor | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
| 103 | `0x1803074d0` | `sub_1803074D0`<br>angle helper predicate | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 104 | `0x180307540` | `sub_180307540`<br>angle helper | `sub` | DELTA | `sub     ecx, cs:dword_18070A610` | `note_us - song_time_us - offset_us`; operands converted, arithmetic unchanged |
| 105 | `0x180307580` | `sub_180307580`<br>combined time helper | `mov` | READ | `mov     eax, cs:dword_18070A610` | `song_time_us`; if ratio, both sides us |
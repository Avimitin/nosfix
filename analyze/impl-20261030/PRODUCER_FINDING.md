# Input-state producer found - 2026-10-30

Parent IDA check of `nostalgia_shared.i64`, resolving the open B6 producer item.

## Result

`sub_180305B30` is the per-frame note-manager update that publishes the staging
input-state buffer. Its signature is:

```c
__int64 __fastcall sub_180305B30(_DWORD *a1, __int64 a2, __int64 a3,
                                 char *a4, char a5, char a6);
```

At `0x180305C6C`:

```asm
mov qword ptr cs:xmmword_1813DEA10, rdi
mov qword ptr cs:xmmword_1813DEA10+8, rsi
```

The decompiler shows `xmmword_1813DEA10 = v7`, with `v7` assembled from
`a2` and `a3`:

```c
*((_QWORD *)&v7 + 1) = a3;
*(_QWORD *)&v7 = a2;
...
xmmword_1813DEA10 = v7;
```

So `0x1813DEA10` is a **two-qword pointer pair**, not a single pointer:

- first qword (`a2`) = the 28 x 28 input-state staging buffer;
- second qword (`a3`) = the companion pointer.

`sub_180307170` reads the first qword at `0x1803071D0` and compacts it into
bucket 0 at `0x1813DEA20`. This is the sequence the build-B `PROBE input`
diff observed.

## Call sites

`sub_180305B30` is called from the note managers, each passing `[object+8]`
as `a2`:

- `0x180197F84` in `sub_180197C30`
- `0x1801AE2CB` in `sub_1801ADFB0`
- `0x1801AE802` in `sub_1801AE480`
- `0x1802FFBA8` in `sub_1802FF8D0`

## Consequence

B6's "producer identity" is closed from bytes. No write watchpoint is needed.
The build-B `PROBE input` lines are a valid read of the staging buffer; their
`state=1/2/3`, `value`, and `repeat` sequence is the real producer output.
The group id is assigned later by `sub_180307170`, which is why every staging
line reports `group=0`.

Implementation can treat `sub_180305B30` as the per-frame producer and
`sub_180307170` as the bucket-0 compactor.

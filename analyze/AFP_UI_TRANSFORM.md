# AFP UI transform hook evidence

The read-only IDA analysis uses a copy of the target `afp-core.dll` from
PAN-2025020501. Its size is 1,673,880 bytes and its SHA-256 is
`03484040F661A8AA49B67D1969C975FF85E4C5D6085347A2DA876AC512549873`.
Earlier notes based on a different crash-folder binary were discarded.
The corresponding target `nostalgia.dll` is 7,640,728 bytes with SHA-256
`339247B90B6DE067AB0DAA4355C7410487BAE9B6F84AD61A4050700FA2D5638B`.

## Per-render call chain

- Nostalgia's `sub_1801C31C0` (RVA `0x1C31C0`) render loop calls
  `sub_1800D6490(0)` at RVA `0x1C35F2` on each render iteration. The wrapper
  stores the zero argument and calls the `afp_core_18` IAT entry with
  `(type=2, id=0)`.
- `afp_core_18` is imported from `afp-core.dll`. On the verified AFP image,
  export ordinal 18 is entry `XCd229cc000011` at RVA `0x17C10`. For type 2 it
  selects active layer records matching ID 0 and calls `sub_180018020` for
  each selected layer.
- `sub_180018020` walks the layer node list at `layer+440`; for each eligible
  node it calls `sub_1800AA670`. That routine calls `sub_1800B8930` at RVA
  `0xAA78C`. The AFP draw routine dispatches `sub_1800B6B90` for display-list
  container tags `0x79` and `0x7B` (callsite RVA `0xB9DD2`).
- Thus the `sub_1800B6B90` traversal containing the patched `0xB6DFB`
  matrix-compose call is reached from the regular per-render export path. The
  source operation at this hook is a node's **local** matrix, composed into
  AFP's current matrix stack; it is not a completed world-matrix snapshot.

## Verified traversal boundary

- `sub_1800B6B90` (RVA `0xB6B90`) is the play-work draw traversal. It starts
  from node `a1`, visits children from `node+72`, and follows sibling links at
  `node+88`. It dispatches a node's `afp_play_work_draw_sub` routine
  (`sub_1800B8930`, RVA `0xB8930`) and recursively draws tag types `0x79` and
  `0x7B`.
- During the per-node loop, the current node is in `RSI`. The code pushes its
  current transform with `sub_180001F70` at RVA `0xB6DF3`, then calls
  `sub_180003080(node)` at RVA `0xB6DFB` (`E8 80 C2 F4 FF`). This exact call
  site has a single node argument and is distinct from other uses of the
  shared helper.
- `sub_180003080` (RVA `0x3080`) reads transform mode bytes at `node+204` and
  `node+205`, the 4x4 local matrix at `node+128..+191`, and pivot coordinates
  at `node+192..+203`. With both mode bytes clear, it multiplies the current
  AFP matrix stack (`qword_180185AC0`) by the node's local matrix. Other modes
  use pivot helpers or do not alter the matrix. The following child draw code
  consumes this pushed state before the enclosing traversal pops it.
- The verified helper prolog is
  `48 83 EC 48 49 89 C8 49 81 C0 80 00 00 00 48 8B 0D 2B 2A 18 00 45 0F B6`.
  The UI integration validates both this prolog and the call instruction before
  adding the callsite redirect. Its module argument is the AFP module, since
  this patch does not live in `nostalgia.dll`.

## Identity and geometry constraints

- Node fields observed by the traversal include type at `+8`, tag/resource
  pointer at `+48`, parent at `+64`, child at `+72`, sibling at `+88`, local
  matrix at `+128`, transform flags at `+204/+205`, and an allocator stamp at
  `+312`.
- `sub_1800A63D0` initializes a work node and writes its stamp at `+312` as
  `(slot_index & 0xFFFF) | ((serial & 0x7FF) << 16) | 0x20000000`. The serial
  advances when a pool allocation is made and wraps after 2,048 allocations.
  The runtime key also includes address, parent, type, and tag/resource pointer;
  absent nodes are pruned after two simulation ticks to limit stale reuse.
- `sub_180003080` only reads through byte `+205`, so the hook can pass it an
  aligned, 208-byte shadow copy with a predicted local matrix and leave the
  original AFP work node unchanged. Prediction is limited to affine
  translation. Pivot modes, invalid matrices, large jumps, ambiguous repeated
  updates, and unavailable history forward the original node unchanged.
- The transform composition is a draw-time operation. The UI timeline and
  `afp_play_work_children_advance` path are not advanced by extra renders.
  Scene identity changes still need to be signaled by the integration when
  available; the per-track gap, key, and jump guards cover ordinary reuse and
  discontinuities visible at this call site.

## Runtime implementation

`include/nosfix/ui_motion.hpp` and `src/ui_motion.cpp` hold the pure bounded
local-transform predictor. `include/nosfix/ui_hooks.hpp` and `src/ui_hooks.cpp` prepare the
single AFP callsite patch, retain per-node histories in a fixed-capacity cache,
and expose presentation/tick boundaries. `tests/ui_motion_tests.cpp` and
`tests/ui_hook_tests.cpp` cover the predictor and a mock callsite invocation;
they do not load or execute game code.

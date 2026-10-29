Nostalgia PAN-2025020501 analysis, 2026-10-29

Read ANALYSIS.txt for conclusions, evidence, limitations, and proposed implementation phases.

nostalgia.dll / nostalgia.dll.i64: copied game DLL and annotated IDA database.
libxact.dll / libxact.dll.i64: copied audio dependency and annotated IDA database.
manifest.json: source paths, sizes, SHA-256 hashes, and source/copy verification.
address_index.json: module-qualified preferred VAs, RVAs, and analyst descriptions.
evidence/: Hex-Rays pseudocode, selected assembly/xrefs/imports, and relevant existing launch-log lines.

Analysis used MCP sessions nos202502 (game) and nosxact (audio). Autoanalysis completed
before inspection. Both sessions were saved and closed after analysis; reopen the .i64 files
for further inspection.
Preferred image base for both files is 0x180000000; each is a separate module.
Runtime addresses require the actual loaded module base plus the recorded RVA.
RVAs are not raw file offsets.

The evidence .c files are decompiler text, NOT compilable or recovered original source.
Types and function names inferred by IDA can be wrong. Critical timing conversions were checked
against assembly; see batchdetail0.json, batchdetail1.json, and insn2.json.
nos202502_0x1802f7c10.c is explicitly truncated by the MCP decompiler tool (123911 characters
reported for the full function). Its call instructions are saved in more1.json; no conclusion
depends on treating its truncated pseudocode as complete.

No game patch, launch, restart, runtime hook, audio-device change, or display change was made.
The source installation was read only. Evidence is static analysis plus its pre-existing log.
The .gitignore excludes local binaries/databases; they are still present on disk for review.

# Legacy state fixtures

`legacy-1.0.1.chunks` contains 37 unmodified DFS1 snapshots captured through
`effGetChunk` on the existing 1.0.1 desktop build before this optimization pass:
12 factories, then five legacy waveform IDs at 60/110/620/2100/2400 Hz.
Each row stores all 51 physical parameter targets. The trailing NUL was replaced
with a newline for this text fixture. These values are regression inputs, not
regenerated expected values from the new mapping.

Original desktop binary SHA-256:
`e4e2b1f7f5da2ca1be5e1f028f7fb1c5667a7b1ab82c76cd46f786f0a2035852`.
Fixture SHA-256:
`760f098e16b4cbd56e0eba8d9a9dfea4bd781b9a16d4ad00a2c4dc1ab8bd85fe`.

`tools/build.sh test` restores every snapshot, compares all physical targets,
and verifies waveform labels against the legacy IDs. The optional argument to
`build/test/performance_controls` also compares directly with an old `.so`.

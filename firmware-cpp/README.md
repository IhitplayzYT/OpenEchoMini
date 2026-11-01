# Echo Mini firmware reconstruction

This is an ongoing portable C++ reconstruction project for the Snowsky
Echo Mini firmware. It contains a read-only RKnano image inspector and
behavior models for selected firmware paths.
It reports header fields, candidate region descriptors, the duplicate header,
and the final 32-bit value without modifying the input image.

```sh
cmake -S . -B build
cmake --build build
./build/echo-fw-inspect ../HIFIEC40.IMG
./build/echo-fw-inspect ../HIFIEC40.IMG --export-images ./exported-assets
```

The exporter writes valid image records as 16-bit `BI_BITFIELDS` BMP files with
RGB565 masks, matching the supplied v3.9 reference assets. It also writes an
`assets.tsv` catalog linking each extracted filename to its metadata index,
ROCK26 index, region-relative pixel offset, and absolute firmware offset.
Firmware pixel words are read high-byte first and written in BMP's little-endian
pixel representation.
The inspector also parses the eight-word module records. It maps non-empty
code and data records to file offsets using the RKnano load-base rule and checks
each extent against the HIFIEC40 file. Pass `--export-modules <directory>` to
write the valid code/data slices and a `modules.tsv` address map. The mapping
matches all 131 non-empty HIFIEC40 code/data ranges; execution semantics still
require analysis.

## Planned layout

- `include/echo/firmware_image.hpp`, `src/firmware_image.cpp`: binary format
  parsing and integrity reporting.
- `include/echo/resources/`: resource metadata, RGB565/BMP conversion, fonts,
  and localized strings.
- `resources/offset_bank_directory.hpp` validates the 21-bank directory in
  header region `0xf4`; `localized_ui_strings.hpp` returns bounded UTF-16LE
  fields for 453 text records in each of 21 locale banks. The decoded catalog
  is exported separately under `reverse-engineering/extracted-regions/`.
- `text/firmware_codepage_tables.hpp` reads the fourteen 16-bit big-endian
  character maps in region `0xc8`. The mapping IDs reflect observed codec
  matches; runtime locale selection is not modeled.
- `include/echo/audio/`: observed codec/container parsers and audio interfaces.
  `capture_ring.hpp` models module 63's sample packing and `0xc000` wrap;
  `module63_transfer.hpp` plans its alternating `0x3000` DMA slots, while
  `module63_control.hpp` preserves selector mappings and ordered mode-command
  helper calls; `module63_block_count.hpp` models the zero-count notification
  and state-derived block quotient; `module63_start.hpp` plans startup flags,
  stream submission, and dependent notifications; `module63_compaction.hpp`
  models its observed two/four-byte sample-pair transformations. These models
  do not invoke firmware callbacks. `module63_capture_tick.hpp` plans the
  readiness gates, pending-input wait/append, submission and transfer-event
  tail; `module63_finalize.hpp` models the bounded 2,000-poll stop transition
  and cleanup flags; `module63_wait.hpp` records the separate state-derived
  wait budget used by the flag-wait helper; `module63_stop.hpp` preserves the
  callback-disable and idle/selector dispatch paths.
  `module73_file_service.hpp` models the observed asynchronous seek/read
  request and per-stream completion bookkeeping, without implementing the MP3
  decoder; `module73_seek_stage.hpp` preserves the caller-side pre-seek wait
  and mode dispatch; `module73_delay.hpp` computes its reciprocal-scaled wait
  count without blocking; `module73_transfer.hpp` models the channel record
  and indexed scatter/gather descriptor chain; `mpeg_layer3_side_info.hpp`
  parses bounded side information and retains observed validation errors;
  `module73_subband_profile.hpp` models the five-way profile selection at a
  separate table-driven subband decoder entry; `mpeg_crc16.hpp` implements the
  observed bounded CRC-16 bit recurrence with caller-supplied seed;
  `module73_layer3_requantize.hpp` ports the packed fixed-point spectral
  magnitude helper and its extracted lookup table;
  `module73_layer3_block_scale.hpp` models the long/start/stop 36-value block
  scaling layouts; `module73_layer3_transform.hpp` models the 18-to-36
  fixed-point transform and window step; `module73_layer3_overlap.hpp` models
  the channel-strided overlap accumulation and history advance;
  `mpeg_huffman_lookup.hpp` decodes the observed bounded root/branch/leaf table
  format.
  Module 27's playback metadata formatter, fixed-position time records, and
  inferred LRC sidecar matching/time-window selection are modeled by
  `format_label.hpp`, `playback_time.hpp`, `lrc_sidecar.hpp`, and
  `playback_block_cursor.hpp`; charset conversion, lyric text extraction, and
  the full screen state machine remain separate work.
- The playback screen header also records the three observed text regions and
  their measured-extent threshold for marquee scrolling; glyph advances and
  the scrolling renderer remain firmware-specific dependencies.
- `display/word_text.hpp` models module 27's standalone 16-bit word copy or
  byte-swap helper. Its callers were not resolved, so it is not assigned to a
  particular encoding path.
- `audio/playback_counter.hpp` returns the observed glyphs and positions for
  module 27's `HH:MM:SS` playback counter after splitting seconds into hours,
  minutes, and seconds. It also records the separate numeric-pair glyph path,
  including its unusual high-value character mapping.
- `module27_playback_screen.hpp` includes the recovered periodic refresh and
  timeout transitions from a separate control-event routine; the firmware
  timebase and service-event behavior remain caller supplied.
- The same header models the confirmed startup event resets from module 27's
  Thumb entry and initializes its watchdog interval/start tick.
- `include/echo/apps/`: module 27's event-gated playback-screen route is
  modeled in `module27_playback_screen.hpp`; screen rendering and state
  handlers remain incomplete. `module5_navigation.hpp` plans the observed
  browser mode, title result, and event `0x17a` state actions.
- `include/echo/device/`: input events, settings, power, storage, and device
  interfaces.
- `include/echo/ebook/`: bounded mixed-width text line wrapping, page rewind,
  module 30's mixed-width unit count and raw text classes, and the current/total
  page indicator with its recovered 100000 overflow threshold.
- `include/echo/radio/`: FM band, tuning, and preset state.
  `module60_audio.hpp` plans the observed sample-rate selection, callback
  buffer sizes, and alternating DMA setup without issuing hardware calls.
  `module60_events.hpp` preserves the event IDs, callback slots, UI signals,
  preset changes, and helper-call conditions from the FM event switch.
  `module62_registers.hpp` models the tuner frequency-byte conversion,
  eight-byte register payload, and seek-status decision without performing
  bus transactions.
  `FmTunerState::set_from_slider_delta` models the firmware's reciprocal-based
  slider mapping, ten-unit rounding, and edge wrap.
- `include/echo/calendar/`: bounded month/year navigation state.
- `include/echo/recording/`: module 28's marker/counter helpers and module 29's
  raw three-state service transition plan; service and capture side effects
  remain caller-owned.
- `include/echo/bitstream/`: bounded MSB-first bit extraction.
- `include/echo/runtime/`: event flags, task-mode table view, module 89 elapsed
  timer/history model, module 90 numeric editor/parser and partial arithmetic
  evaluator, plus unsigned quotient/remainder division and module 0 soft-float
  integer/binary32-to-binary64 conversion helpers. `module0_memory.hpp` also
  models the module 0 fill, overlap-safe move, and bounded string-copy loops.
  `module0_restart.hpp` records the observed low-level restart sequence without
  issuing device writes or restarting the host. `module0_exception_dump.hpp`
  formats the shared exception handler's PC/LR/MSP/PSP output without UART I/O.
  `module0_vector_table.hpp` models its 57-slot runtime vector table.
  `module0_interrupt_control.hpp` describes the controller register actions
  used to clear, enable, and disable vector slots.
  `module0_irq_guard.hpp` plans the observed nested global-interrupt and IRQ 5
  masking actions without executing privileged instructions or MMIO.
  `module0_timer_callback.hpp` models the per-node elapsed/period/count update
  and reports when a callback is due or its node expires.
  `module0_sample_mux.hpp` models the vector 36 sample store, pending bits,
  byte command ring, and next-channel control action without MMIO.
  `module0_dma.hpp` plans the observed requested-channel/fallback choice
  (fallback scan is channels 0–4) and bounded `0x0ff8`-byte transfer
  segmentation without programming the controller.
- `include/echo/image/`: BMP metadata and row decoding, JPEG frame/Huffman
  primitives, and module 19's reduced fixed-point transform.
- `include/echo/display/`: bitmap-font helpers and module 4's LCD window-byte
  encoder.
- `display/module0_rtl_text.hpp` reproduces module 0's bounded Hebrew-run
  ordering heuristic; it is not a general Unicode bidirectional engine.
- `runtime/module0_memory.hpp` models module 0's byte fill and bounded
  zero-padded string-copy helpers, including calls shared into the image overlay.
- `include/echo/storage/`: block, directory, path, record and CUE text helpers.
  `module8_transfer.hpp` preserves the file/device overlay's alternating
  16-sector callback and device-read steps without assigning a direction to
  its unresolved module-entry operation.
- `include/echo/usb/`: bounded stream ring/transfer blocks and Mass Storage
  command-wrapper parsing.
- `include/echo/apps/`: reconstructed Music, Browser, Radio, Recorder, Video,
  Picture, Ebook, Settings, and USB modes.
- `docs/`: evidence ledger, checkpoint history, feature traces, and unresolved
  questions.

Observed device behavior will be separated from emulator implementation
choices. No firmware rewriting or flashing is implemented in this foundation.

The first behavior model is `echo::device::UsbInterruptModel`. It preserves the
USB status-bit transitions and ordered status writes recovered from module 0;
unknown controller masks and peripheral effects remain caller-supplied or
unmodeled.

Other recovered slices include bounded audio container/header readers, a
capture ring, storage directory ordering and indexing, event flags, bitmap font
helpers, embedded image discovery, FM tuner state, e-book line layout,
calendar month navigation, and an MSB-first video bitstream primitive.
Image helpers now include a bounded JPEG frame descriptor, Huffman symbol
reader, reduced fixed-point JPEG component-block transform, indexed-color BMP
metadata and palette-extent parser, bounded source-pixel converter, and BMP
row decoder. The BMP decoder handles row padding and top-down or bottom-up
traversal for the observed 1-, 4-, and 8-bit uncompressed subset; it returns
RGB components before the firmware's external packed-pixel callback.
Module 20's nearest-sample 16.16 scale-axis cursor is represented separately.
Full clipping, row placement, callback output packing, and JPEG
entropy-to-block integration remain incomplete.
The USB stream model covers
buffering only, not endpoint or transfer
control behavior. Storage utilities also include a caller-comparator fixed
record index and the auxiliary payload byte transform.
The audio path includes ID3 and MPEG frame-header parsing, the observed
Layer III reservoir block-copy primitive, module 73's recovered Huffman
codebook descriptor/tree data, bounded pair and count1-quadruple decoding,
bounded MPEG-1/LSF scalefactor reader, exact module 73 long/short/mixed band
width tables, and a bounded long-granule decoder for supported non-short
blocks under `part2_3_length`, plus short/mixed granule decoding with
window-specific line exponents. Fixed-point spectral helpers include exponent
calculation and bounded Huffman traversal, plus side-info-order granule
sequencing and caller-sized `main_data_begin` window assembly; exact firmware
reservoir sizing, mode-flag derivation from the MPEG header and PCM synthesis
remain unfinished. MPEG-1/LSF intensity stereo and masked mid/side processing
are included behind explicit mode inputs. The project
also includes a
target-limited FLAC frame-header parser, and
FLAC channel restoration, but not full codec decode or PCM synthesis.
Module 75 also has a bounded MPEG-4 AAC descriptor parser for tags `0x03`,
`0x04`, and `0x05`; it returns the AudioSpecificConfig frequency and channel
table indices without guessing the relocated firmware table values, plus a
Program Config Element parser for its channel-element lists, mixdown settings,
and comment span, a Channel Pair Element header parser for common-window ICS
and mid-side masks, plus SCE/LFE element-tag parsing and an Individual Channel
Stream info parser for
long/short windows, grouping, and predictor flags. AAC Huffman traversal has a bounded
count-by-depth symbol walker and a section-data parser that expands codebook
runs across window groups; a scalefactor parser applies caller-provided tables
for normal, noise, and intensity bands. DSE and FIL parsers preserve bounded
payload spans at arbitrary bit offsets; pulse data, long/short TNS syntax,
table-driven gain-control syntax, packed spectral tuple/sign/escape parsing,
one-band coefficient assembly, and pulse correction are also decoded.
The Q15 saturation, signed high-word product, and spectral-tail clear helpers
are also ported. Relocated codebook and transform tables remain caller-supplied.
The MOV reader has a separate bounded classic atom-prefix parser in
`echo/container/module75_mov_atoms.hpp`.
Module 74's WMA codec-format payload parser covers tags `0x160` through
`0x163`, minimum payload lengths, channel restrictions, firmware format-class
selection, valid-bit alignment, and the fixed extensible-format state bytes.
The surrounding ASF object GUID routing and sample decoder are still open.
`echo::audio::plan_music_service_message` records module 33's sparse
message routing and known argument/state effects; runtime shared-state access
and audio/UI handlers still need an adapter.
Module 33's stereo fixed-point sample recurrence and rate/mode coefficient
selection are also represented as partial plans, along with the caller's
ordered setup/event effects. Coefficient table contents and external event,
shared-memory, and cleanup helpers remain unresolved.
The same module now has a bounded ASF Header Object child walker and dispatch
for its two metadata objects, including the observed Content Description fields
and Extended Content Description `WM/*` tag routing. Its ring-buffer cursor,
refill arithmetic and 0x100-byte skips are partly modeled; the indirect file
source callback remains an adapter boundary.
These are partial behavior models; firmware encoding tables, peripheral
effects, large codec/DSP routines, and full application screens remain
unfinished.

Two focused models were added from subsequent code slices:

- `echo::audio::CaptureRing` reproduces module 63's observed 3/4-byte word
  packing and `0xc000` ring wrap. It does not model the full transfer engine.
- `echo::storage::DirectoryComparator` reproduces module 40's four-field and
  optional UTF-16 name ordering. It does not decode on-disk entries or perform
  block I/O.
- `echo::storage::DirectoryPrefixRecord` reads the observed four-unit UTF-16
  name prefix and 16-bit name-record index from a 14-byte runtime node. It does
  not parse on-disk directory records.

Module 81 also contains a DSDIFF/DST file path. `echo::audio::DsdiffParser`
extracts container metadata and data ranges, including the observed 9,408-byte
raw DSD block size. It does not decode audio samples or reproduce the firmware's
DSP routines.

Additional parser primitives now include `echo::audio::WavContainerParser`
for RIFF/WAVE `fmt ` and `data` metadata, and `echo::audio::ApeBitReader` for
bounded MSB-first bit reads observed in module 80. These do not implement PCM,
ADPCM, or APE sample decoding.

`echo::runtime::EventFlagBank` models module 0's 32-bit-word event bitset with
bounded test, set, clear, and test-and-clear operations. Event IDs are known at
the storage level, but their names and producers are still being traced.

`echo::runtime::Module0CleanupList` models the observed doubly linked callback
list's forward membership check and tail-to-head cleanup order. Its node type
is a host-side abstraction, not the firmware's original pointer layout.

`echo::runtime::advance_module0_usb_detection` models module 0's armed USB
detection poll: success disarms immediately, while a timeout requires more
than 300 unsigned ticks and is surfaced once. The tick frequency and surrounding
service side effects are unresolved.

`echo::display::split_module0_dirty_rows` models the module 0 renderer's
inclusive row-range split at row 140. Display callbacks and framebuffer memory
remain platform-owned.

These C++ slices are hand-built behavior models based on static analysis. They
are not original firmware source. The project compiled at checkpoint 214; no
tests were run.

`echo::apps::plan_module17_progress_band` models the observed priority and draw
arguments in module 17's progress-band helper. The touch/key event router and
its shared records remain incomplete.

`echo::recording::compute_module22_selection_geometry` and
`plan_module22_selection_change` model module 22's selection normalization,
six-entry range arithmetic, six-byte visible-row reads, bounded selection
updates, redraw calls, and event notifications. The third record word and
file-opening behavior remain unresolved. Its nine observed record triples are
available through `module22_firmware_records()`.

`echo::storage::hash_module23_name` reproduces the module 23 UTF-16 code-unit
FNV-style hash with ASCII-only case folding. Its companion comparator and
cache probe preserve the observed full-name check and 0/1/2 status values;
they do not perform filesystem I/O. `hash_module23_catalog_name16` models the
separate 16-bit multiply-by-31 name index used by the catalog coordinator.
`Module23NameCacheEntry` groups the observed parallel-array fields into one
host-side value and does not represent the firmware's original memory layout.
`plan_module23_sort_info_check` describes the sort verifier's byte-read
sequence and expected-word comparison without performing I/O.
`compute_module23_sort_menu_geometry` reproduces its six-row page selection
and tail-three-row case for selection code 14.

`echo::storage::read_module31_utf16_word` models module 31's 0x400-byte
buffered two-byte reader with an injected file-refill callback, cursor, EOF,
and byte-order state. It does not open files or parse playlist syntax.
`detect_module31_bom` recognizes the three BOM forms checked by the playlist
loader, and `plan_module31_playlist_line` preserves its ASCII whitespace and
comment handling plus explicit-path fallback order. Filesystem lookup and
non-ASCII byte conversion remain external to this model.
`split_module31_path` models the five-component staging and 63-code-unit
component limit before browser lookup.

`echo::apps::step_module32_setting` models module 32's bounded up/down
selection byte, two conditional external handler dispatches, and redraw event
`0x3e`.
`plan_module32_route_selection` preserves the selector-to-word table and
conditional service start/stop transition without naming the route words or
service helpers.
`step_module32_cyclic_value` implements the shared two-limit wrap arithmetic
used by module 32's value controls.

`echo::storage::convert_module38_time_field` reproduces the CUE parser's
time-like record conversion to milliseconds, including the 74-frame cap.
`parse_module38_tag_envelopes` reads its bounded signature/kind/value records
without assigning meanings to the numeric fields.
`read_module38_string_pair` preserves the second record form's two lengths,
six opaque header bytes, and `0x80`-byte output caps.
`Module38StreamBuffer` reproduces module 38's 256-byte refill/compaction and
chunked skip state around an injected reader callback.
`read_module38_line_record` models CRLF scanning and record-class trim lengths
without assigning the line grammar.

`select_module40_bucket` reproduces the directory index's linear threshold
scan when given a normalized key and threshold span.
`plan_module40_prefix_read` computes module 40's two 16-byte/8-byte storage
read offsets for its observed region modes.
`DirectoryPrefixRecord::decode_prefix_bytes` turns the eight-byte read into
four normalized UTF-16 prefix units.
`plan_module40_listed_prefix_read` computes the first reader's indexed or
sequential `0x800`-byte prefix block offset.
`module40_primary_index_passes` preserves the four first-stage worker modes,
raw constants and workspace offsets from the top-level index builder.

`compare_module59_byte_strings` preserves module 59's bounded byte comparison,
ASCII folding, filter flags, and equal-byte count using bounded host spans.
`transform_module59_record` applies its three observed transform record types
using the exact 512-byte lookup region extracted from the firmware.
`Module59TransformMap` models sorted transform ranges, overlap replacement,
gap lookup, and adjacent descriptor coalescing. The surrounding container
header prefix checks are modeled by `parse_module59_header_prefix`; secondary
metadata probing, row access, and the file API are still being reconstructed.

`echo::transport::plan_module51_command` models the module 51 dispatcher,
bounded event waits, return codes, and two-buffer transfer setup. It exposes
the transfer's no-timeout completion wait as an adapter action.
Modules 53, 54, 56, 57, and 58 have named APIs where their dispatcher behavior
matches module 51; module 55's plan additionally marks its helper-status wait
exit on commands `0x0e` and `0x0f`.

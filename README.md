# SubSync — Modernized PT-BR Fork

> Automatic subtitle synchronization via speech recognition and time-series correlation.

This is a **modernized, Brazilian Portuguese (PT-BR) optimized fork** of the now-deprecated [`sc0ty/subsync`](https://github.com/sc0ty/subsync). The original project is no longer maintained ([see #197](https://github.com/sc0ty/subsync/issues/197)). This fork eradicates legacy bugs, upgrades every major dependency, and re-engineers the speech pipeline to accurately synchronize subtitles against fast conversational Brazilian Portuguese audio — including regional slang, informal contractions, and heavily accented dialogue.

---

## What's New

### Speech Engine: PocketSphinx → Vosk
The legacy PocketSphinx engine produced unacceptable word-error rates (>40%) on informal PT-BR speech. This fork replaces it with [Vosk](https://alphacephei.com/vosk/), an offline speech recognition toolkit with a dedicated Brazilian Portuguese model (`vosk-model-pt-fb-v0.1.1`) trained on real-world conversational data. The integration is surgical — only the `SpeechRecognition` class changed — and PocketSphinx can still be compiled in via the `USE_VOSK=OFF` CMake flag for backward compatibility.

### FFmpeg 5.x / 6.x / 7.x Compatibility
Every deprecated FFmpeg API call has been migrated:
- `av_init_packet` → `av_packet_alloc` / `av_packet_free`
- `avcodec_close` → `avcodec_free_context`
- `ctx->channels` / `ctx->channel_layout` → `ctx->ch_layout` (AVChannelLayout API)
- `swr_alloc_set_opts` → `swr_alloc_set_opts2`
- `AVCodec*` / `AVInputFormat*` → `const` qualified

All changes are version-guarded so the code still compiles against FFmpeg 4.x.

### Accent-Insensitive Word Matching for PT-BR
The original `compareWords()` function used **prefix-only matching** that terminated at the first character mismatch. A single diacritic difference (`ação` vs `acao`, `você` vs `voce`) killed the correlation point. The new algorithm uses three-tier scoring:
1. **Exact codepoint match** → +2.0
2. **Case-insensitive match** → +1.5
3. **Accent-insensitive match** (via Unicode normalization) → +1.2
4. **Full mismatch** → −0.5 penalty (but does **not** break early)

This alone dramatically improves correlation quality for Portuguese, Spanish, French, and any Latin-script language with diacritics.

### Brazilian Portuguese as First-Class Language
- `pob` is now a dedicated `LanguageInfo` entry (not just an alias of `por`)
- Extra codes: `pt-br`, `pt_BR`, `ptbr`, `pb`
- Default encoding: UTF-8 first (modern BR subtitles are overwhelmingly UTF-8)
- Separate speech model asset key: `speech/pob.speech`

### Modern Build System
- **`pyproject.toml`** (PEP 517/518) replaces the `distutils`-dependent `setup.py` as the primary build configuration
- **`CMakeLists.txt`** provides a proper CMake build for the C++ `gizmo` extension
- **C++17** standard (up from C++11/14)
- **Python ≥ 3.10** (the old `distutils` imports broke on Python 3.12+)
- All Python dependencies updated to current versions

### Critical Bug Fixes
| Bug | Severity | Fix |
|-----|----------|-----|
| `SpeechRecognition::flush()` was empty — dropped final audio buffer at EOS | Critical | Implemented proper drain for both Vosk and Sphinx |
| `Line(Point, Point)` divided by zero on vertical/duplicate points | High | Epsilon guard with safe fallback |
| `AudioDec` / `SubtitleDec` empty destructors leaked codec contexts on partial init failure | High | RAII cleanup in destructors |
| `SpeechRecognition::start()` used `frate` as divisor before null-checking it | Critical | Check moved before division |
| `cmdargs.parseCmdArgs()` returned `None` on error → downstream `AttributeError` | High | Returns `{}` on failure |
| `ErrorsGroup.add` used `repr(str)` instead of `repr(err)` | Medium | Fixed |
| `encdetect.detectEncoding` fell off without `return` | Medium | Explicit `return None` |
| `settings.__eq__` used `dict_keys \| dict_keys` (Python 3.9+ only) | Medium | Always returns `set` |
| `loggercfg` monkey-patched `threading.Thread.__init__` globally | Low | Uses `threading.excepthook` (Python 3.8+) |
| `Utf8::toUpper` defined outside its namespace → linker error | Medium | Fixed to `Utf8::toUpper` |

---

## Requirements

- **Python** ≥ 3.10
- **FFmpeg** development libraries (`libavformat`, `libavcodec`, `libavutil`, `libswresample`)
- **Vosk** (`libvosk`) — or PocketSphinx for legacy builds
- **pybind11** ≥ 2.11
- **wxPython** ≥ 4.2 (for GUI mode)

## Building

### With CMake (recommended)

```bash
pip install pybind11
mkdir build && cd build
cmake .. -DUSE_VOSK=ON
make -j$(nproc)
```

### With pip

```bash
pip install -e ".[gui]"
```

## Usage

### CLI — Sync PT-BR subtitles to PT-BR audio

```bash
subsync --cli sync \
  --sub legenda.srt --sub-lang pob \
  --ref video.mkv --ref-lang pob --ref-stream-by-type audio \
  --out synced.srt
```

### PT-BR Vosk Speech Model Setup

1. Download a Vosk Portuguese model (e.g. [`vosk-model-pt-fb-v0.1.1-20220516`](https://alphacephei.com/vosk/models))
2. Create a `.speech` descriptor JSON:

```json
{
    "version": "1.0.0",
    "vosk": {
        "model-path": "./vosk-model-pt-fb-v0.1.1-20220516"
    },
    "samplerate": 16000,
    "sampleformat": "S16"
}
```

3. Place it in the assets directory as `speech/pob.speech`

## License

GPLv3 — same as the original project.

## Credits

Original project by [Michał Szymaniak (sc0ty)](https://github.com/sc0ty/subsync).

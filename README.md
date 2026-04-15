# SubSync PT-BR — Modernized Subtitle Synchronizer

> Automatic subtitle synchronization via speech recognition and time-series correlation.

This is a **modernized, Brazilian Portuguese (PT-BR) optimized fork** of the now-deprecated [`sc0ty/subsync`](https://github.com/sc0ty/subsync). The original project is no longer maintained ([see #197](https://github.com/sc0ty/subsync/issues/197)). This fork eradicates legacy bugs, upgrades every major dependency, and re-engineers the speech pipeline to accurately synchronize subtitles against fast conversational Brazilian Portuguese audio — including regional slang, informal contractions, and heavily accented dialogue.

---

## Table of Contents

- [What's New](#whats-new)
- [Installation](#installation)
- [Usage](#usage)
  - [GUI Mode](#gui-mode)
  - [CLI Mode](#cli-mode)
  - [Batch Processing](#batch-processing)
- [Supported Formats](#supported-formats)
- [Supported Languages](#supported-languages)
- [Configuration](#configuration)
- [Building from Source](#building-from-source)
- [Bug Fixes](#bug-fixes)
- [License](#license)

---

## What's New

### Speech Engine: PocketSphinx → Vosk
The legacy PocketSphinx engine produced unacceptable word-error rates (>40%) on informal PT-BR speech. This fork replaces it with [Vosk](https://alphacephei.com/vosk/), an offline speech recognition toolkit with a dedicated Brazilian Portuguese model. The integration is surgical — only the `SpeechRecognition` class changed — and PocketSphinx can still be compiled in via the `USE_VOSK=OFF` CMake flag for backward compatibility.

### ASS/SSA Subtitle Support
Full `.ass` and `.ssa` subtitle support with **styling preservation**. When synchronizing ASS subtitles, all formatting (fonts, colors, effects, karaoke tags) is preserved — only the timing changes. Batch output patterns support `{sub_ext}` to automatically match input format.

### Accent-Insensitive Word Matching
The original `compareWords()` used prefix-only matching that broke on diacritics (`ação` vs `acao`). The new three-tier algorithm:
1. **Exact match** → +2.0
2. **Case-insensitive** → +1.5
3. **Accent-insensitive** (Unicode normalization) → +1.2
4. **Mismatch** → −0.5 penalty (no early break)

### FFmpeg 5.x / 6.x / 7.x Compatibility
Every deprecated FFmpeg API call has been migrated with version guards for backward compat.

### Brazilian Portuguese as First-Class Language
- `pob` is a dedicated language entry (not just `por` alias)
- Extra codes: `pt-br`, `pt_BR`, `ptbr`, `pb`
- Pre-bundled Vosk PT-BR model in portable builds
- Default encoding: UTF-8

### Modern Build System
- **`pyproject.toml`** (PEP 517/518) replaces `distutils`
- **C++17**, **Python ≥ 3.10**
- All dependencies updated to current versions

---

## Installation

### Windows (Portable)
Download the latest portable `.exe` from [Releases](https://github.com/CoolFreeze23/subsync-ptbr/releases). No installation required — just run it. The Portuguese speech model is bundled.

### Windows (MSI Installer)
Download the `.msi` installer from Releases. Installs alongside the original `subsync` without conflicts.

### From Source
See [Building from Source](#building-from-source).

---

## Usage

### GUI Mode
Simply run `subsync-ptbr` (or the portable exe) without arguments to launch the graphical interface.

1. Select your **subtitle file** (`.srt`, `.ass`, `.ssa`, `.sub`)
2. Select your **reference** (video/audio file, or another subtitle)
3. Set the correct **languages** for both
4. Click **Start** and wait for synchronization
5. Save the result

### CLI Mode

Basic synchronization:
```bash
subsync-ptbr --cli sync \
  --sub legenda.srt --sub-lang pob \
  --ref video.mkv --ref-lang pob --ref-stream-by-type audio \
  --out synced.srt
```

ASS subtitle (styling preserved):
```bash
subsync-ptbr --cli sync \
  --sub styled.ass --sub-lang eng \
  --ref video.mkv --ref-lang eng --ref-stream-by-type audio \
  --out synced.ass
```

With custom effort and offset:
```bash
subsync-ptbr --cli sync \
  --sub input.srt --sub-lang pob \
  --ref video.mkv --ref-lang pob --ref-stream-by-type audio \
  --out output.srt \
  --effort=1.0 --out-time-offset=-0.5
```

### CLI Options Reference

| Option | Description |
|--------|-------------|
| `--cli`, `-c` | Run in headless (CLI) mode |
| `--sub PATH` | Input subtitle file |
| `--sub-lang LANG` | Subtitle language (3-letter code, e.g. `eng`, `pob`) |
| `--ref PATH` | Reference file (video, audio, or subtitle) |
| `--ref-lang LANG` | Reference language |
| `--ref-stream-by-type TYPE` | Select stream by type: `audio` or `sub` |
| `--ref-stream NO` | Select stream by number |
| `--out PATH` | Output subtitle path |
| `--out-enc ENC` | Output character encoding (default: UTF-8) |
| `--effort FLOAT` | Synchronization effort 0.0-1.0 (default: 0.5) |
| `--window-size SECS` | Max time correction window in seconds (default: 1800) |
| `--max-point-dist SECS` | Max acceptable sync error in seconds (default: 2.0) |
| `--min-points-no N` | Minimum synchronization points (default: 10) |
| `--min-word-len N` | Minimum word length for matching (default: 5) |
| `--out-time-offset SECS` | Add constant offset to output (default: 0.0) |
| `--overwrite` | Overwrite existing output files |
| `--verbose N` | Verbosity level (0-3, default: 1) |
| `--offline` | Don't download assets, use only local |
| `-j N`, `--jobs N` | Number of concurrent sync threads |
| `--loglevel LEVEL` | DEBUG, INFO, WARNING, ERROR, CRITICAL |
| `--logfile PATH` | Write logs to file |

### Batch Processing

**GUI**: Use the batch view (Menu → Batch) or start with `--batch`. Drag & drop multiple files.

**CLI**: Run multiple `sync` commands:
```bash
subsync-ptbr --cli \
  sync --sub ep01.srt --sub-lang pob --ref ep01.mkv --ref-lang pob --ref-stream-by-type audio --out ep01.synced.srt \
  sync --sub ep02.srt --sub-lang pob --ref ep02.mkv --ref-lang pob --ref-stream-by-type audio --out ep02.synced.srt
```

**Output patterns** for batch:
- `{sub_name}` / `{ref_name}` — filename without extension
- `{sub_dir}` / `{ref_dir}` — directory path
- `{sub_ext}` / `{ref_ext}` — file extension (e.g. `srt`, `ass`)
- `{sub_lang}` / `{ref_lang}` — 3-letter language code
- `{if:field:value}` — conditional append

Example pattern: `{ref_dir}/{ref_name}{if:sub_lang:.}{sub_lang}.{sub_ext}`

### Keyboard Interrupt (Ctrl+C)

In CLI mode, pressing **Ctrl+C** during synchronization will:
1. Gracefully terminate the sync
2. If enough correlation points exist, **save a partial result** before exiting
3. Report what was saved

---

## Supported Formats

### Subtitle Input
| Format | Extension | Styling Preserved |
|--------|-----------|-------------------|
| SubRip | `.srt` | N/A (text only) |
| Advanced SubStation Alpha | `.ass` | Yes |
| SubStation Alpha | `.ssa` | Yes |
| MicroDVD | `.sub` | — |

### Reference Input
Any video or audio format supported by FFmpeg (`.mkv`, `.mp4`, `.avi`, `.mp3`, `.flac`, `.wav`, etc.)

---

## Supported Languages

Any language with a Vosk model can be used. The portable build bundles:
- **Portuguese (Brazil)** — `pob` / `por`

Additional models can be downloaded from [Vosk Models](https://alphacephei.com/vosk/models) and placed in the assets directory.

Pre-existing dictionary-based support (subtitle ↔ subtitle sync):
English, Spanish, French, German, Italian, Dutch, Polish, Czech, Russian, Chinese, Japanese, Korean, Arabic, Turkish, and many more.

---

## Configuration

Settings are stored in `subsync-ptbr.json` (platform-dependent location).

Key settings editable via the config file:
```json
{
    "defaultSubLang": "pob",
    "defaultRefLang": "pob",
    "defaultView": "batch",
    "minPointsNo": 10,
    "minEffort": 0.5,
    "windowSize": 1800.0,
    "logLevel": 30
}
```

| Setting | Description | Default |
|---------|-------------|---------|
| `defaultSubLang` | Pre-fill subtitle language | `null` |
| `defaultRefLang` | Pre-fill reference language | `null` |
| `defaultView` | Start in `"basic"` or `"batch"` mode | `"basic"` |
| `minPointsNo` | Min sync points needed | `10` |
| `minEffort` | When to stop (0-1) | `0.5` |
| `windowSize` | Max time shift in seconds | `1800` |

---

## Building from Source

### Requirements
- **Python** ≥ 3.10
- **FFmpeg** development libraries (`libavformat`, `libavcodec`, `libavutil`, `libswresample`)
- **Vosk** (`libvosk`) — or PocketSphinx for legacy builds
- **pybind11** ≥ 2.11
- **wxPython** ≥ 4.2 (for GUI mode)
- **MSVC** (Windows) or **GCC/Clang** (Linux/macOS)

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

### Windows (MSVC)

```powershell
# Set up MSVC environment
& "C:\Program Files\Microsoft Visual Studio\...\vcvarsall.bat" x64

# Set environment variables
$env:VOSK_DIR = "C:\path\to\vosk"
$env:USE_PKG_CONFIG = "no"

# Build
python setup.py build_ext --inplace
```

### Building Portable EXE

```bash
pip install pyinstaller
pyinstaller windows.spec
```

Output: `dist/subsync-ptbr-portable.exe`

---

## Bug Fixes (vs Original)

| Bug | Severity | Fix |
|-----|----------|-----|
| PocketSphinx >40% WER on PT-BR | Critical | Replaced with Vosk |
| Last subtitle extends to end of video (#188) | High | Duration clamping on transformed events |
| `Invalid output pattern` with `{}` in filenames (#182, #97) | High | Graceful handling of unknown pattern variables |
| CLI spawns GUI console window on Windows (#191) | Medium | Only allocate console when stdout unavailable |
| No error message when CLI sync fails (#160) | Medium | Clear message with point count and suggestions |
| No way to Ctrl+C and save partial result (#135) | Medium | SIGINT handler saves partial sync if correlated |
| Small offset (<2s) subtitles fail to sync (#164) | Medium | Lowered default `minPointsNo` from 20 to 10 |
| ASS styling lost on sync (always saves as .srt) | High | Preserves format and all styling |
| `Slider.SetValue` float TypeError crashes GUI | High | All GUI values cast to int |
| `Line(Point, Point)` division by zero | High | Epsilon guard |
| `AudioDec`/`SubtitleDec` leak codec contexts | High | RAII cleanup |
| `SpeechRecognition::flush()` was empty | Critical | Proper drain implemented |
| `Settings.__eq__` fails on Python 3.9+ | Medium | Returns `set` |
| `loggercfg` monkey-patches `Thread.__init__` | Low | Uses `threading.excepthook` |
| Vosk DLL not found in portable EXE | High | Explicit DLL bundling in PyInstaller spec |

---

## License

GPLv3 — same as the original project.

## Credits

Original project by [Michał Szymaniak (sc0ty)](https://github.com/sc0ty/subsync).
Fork maintained by [CoolFreeze23](https://github.com/CoolFreeze23/subsync-ptbr).

# SubSync architecture overview
SubSync is composed of two main modules:
- main module written in Python;
- _gizmo_ written in C++ and compiled to Python binary module.

Main module is responsible for displaying GUI, asset downloading and installation and contains business logic, whereas gizmo is doing heavy lifting such as speech recognition, audio and subtitle extraction and synchronization.

## Main module
Written in Python (3.10 or newer).

## Gizmo
Written in C++17 with [pybind11](https://github.com/pybind/pybind11), compiled as Python binary module. Uses [FFmpeg](https://ffmpeg.org) libraries for media extraction and [Vosk](https://alphacephei.com/vosk/) for speech recognition (with legacy PocketSphinx support via compile flag).

### Speech Recognition Pipeline
Audio → FFmpeg Demux → Audio Decode → Resample (S16/16kHz/Mono) → Vosk Recognizer → Word timestamps with confidence → Correlator

### Word Comparison
The `compareWords` function uses a three-tier similarity scoring:
1. Exact codepoint match (+2.0)
2. Case-insensitive match (+1.5)
3. Accent-insensitive match via Unicode normalization (+1.2)
4. Mismatch penalty (-0.5, does NOT terminate early)

This ensures proper correlation for languages with diacritics (Portuguese, French, Spanish, etc.).

# -*- mode: python ; coding: utf-8 -*-

import os

ffmpeg_bin = r'C:\dev\ffmpeg\ffmpeg-master-latest-win64-gpl-shared\bin'
vosk_pkg = os.path.join(
    r'C:\Users\Alvin\AppData\Local\Programs\Python\Python312\Lib\site-packages', 'vosk')
vosk_model = r'C:\ProgramData\subsync-ptbr\assets\speech'
dict_dir   = r'C:\ProgramData\subsync-ptbr\assets\dict'

extra_binaries = [
    (os.path.join(ffmpeg_bin, 'avcodec-62.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'avformat-62.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'avutil-60.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'swresample-6.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'swscale-9.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'avdevice-62.dll'), '.'),
    (os.path.join(ffmpeg_bin, 'avfilter-11.dll'), '.'),
    (os.path.join(vosk_pkg, 'libvosk.dll'), 'vosk'),
    (os.path.join(vosk_pkg, 'libgcc_s_seh-1.dll'), 'vosk'),
    (os.path.join(vosk_pkg, 'libstdc++-6.dll'), 'vosk'),
    (os.path.join(vosk_pkg, 'libwinpthread-1.dll'), 'vosk'),
    ('gizmo.cp312-win_amd64.pyd', '.'),
]

main_a = Analysis(['bin/subsync'],
        pathex=['.'],
        binaries=extra_binaries,
        datas=[
            ('LICENSE', '.'),
            ('subsync/key.pub', '.'),
            ('subsync/img', 'img'),
            ('subsync/locale', 'locale'),
            (os.path.join(vosk_model, 'por.speech'), 'assets/speech'),
            (os.path.join(vosk_model, 'vosk-model-small-pt-0.3'), 'assets/speech/vosk-model-small-pt-0.3'),
            (os.path.join(vosk_model, 'eng.speech'), 'assets/speech'),
            (os.path.join(vosk_model, 'vosk-model-small-en-us-0.15'), 'assets/speech/vosk-model-small-en-us-0.15'),
            (os.path.join(dict_dir, 'eng-por.dict'), 'assets/dict'),
            ],
        hiddenimports=[],
        hookspath=[],
        runtime_hooks=[],
        excludes=[],
        win_no_prefer_redirects=False,
        win_private_assemblies=False,
        cipher=None,
        noarchive=False)

main_pyz = PYZ(main_a.pure, main_a.zipped_data, cipher=None)

main_exe = EXE(main_pyz,
        main_a.scripts,
        [],
        exclude_binaries=True,
        name='subsync-ptbr',
        debug=False,
        bootloader_ignore_signals=False,
        strip=False,
        upx=True,
        console=False,
        icon='resources/icon.ico')

main_dbg = EXE(main_pyz,
        main_a.scripts,
        [],
        exclude_binaries=True,
        name='subsync-ptbr-debug',
        debug=True,
        bootloader_ignore_signals=False,
        strip=False,
        upx=True,
        console=True,
        icon='resources/icon.ico')

main_cmd = EXE(main_pyz,
        main_a.scripts,
        [],
        exclude_binaries=True,
        name='subsync-ptbr-cmd',
        debug=False,
        bootloader_ignore_signals=False,
        strip=False,
        upx=True,
        console=True,
        icon='resources/icon.ico')

main_coll = COLLECT(main_exe,
        main_dbg,
        main_cmd,
        main_a.binaries,
        main_a.zipfiles,
        main_a.datas,
        strip=False,
        upx=True,
        upx_exclude=[],
        name='subsync-ptbr')

portable_a = Analysis(['bin/portable'],
        pathex=['.'],
        binaries=extra_binaries,
        datas=[
            ('LICENSE', '.'),
            ('subsync/key.pub', '.'),
            ('subsync/img', 'img'),
            ('subsync/locale', 'locale'),
            (os.path.join(vosk_model, 'por.speech'), 'assets/speech'),
            (os.path.join(vosk_model, 'vosk-model-small-pt-0.3'), 'assets/speech/vosk-model-small-pt-0.3'),
            (os.path.join(vosk_model, 'eng.speech'), 'assets/speech'),
            (os.path.join(vosk_model, 'vosk-model-small-en-us-0.15'), 'assets/speech/vosk-model-small-en-us-0.15'),
            (os.path.join(dict_dir, 'eng-por.dict'), 'assets/dict'),
            ],
        hiddenimports=[],
        hookspath=[],
        runtime_hooks=[],
        excludes=[],
        win_no_prefer_redirects=False,
        win_private_assemblies=False,
        cipher=None,
        noarchive=False)

portable_pyz = PYZ(portable_a.pure, portable_a.zipped_data, cipher=None)

portable_exe = EXE(portable_pyz,
        portable_a.scripts,
        portable_a.binaries,
        portable_a.zipfiles,
        portable_a.datas,
        [],
        name='subsync-ptbr-portable',
        debug=False,
        bootloader_ignore_signals=False,
        strip=False,
        upx=True,
        upx_exclude=[],
        runtime_tmpdir=None,
        console=False,
        icon='resources/icon.ico')

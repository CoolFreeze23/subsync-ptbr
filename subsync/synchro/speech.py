import os
import gizmo
from subsync import assets
from subsync import error
from subsync.translations import _

import logging
logger = logging.getLogger(__name__)

try:
    from vosk import Model as VoskModel
    HAS_VOSK_PYTHON = True
except ImportError:
    HAS_VOSK_PYTHON = False


def loadSpeechModel(lang):
    logger.info('loading speech recognition model for language %s', lang)

    asset = assets.getAsset('speech', [lang])
    if asset.localVersion():
        model = asset.readSpeechModel()
        logger.debug('model ready: %s', model)
        return model

    raise error.Error(_('There is no speech recognition model for language {}')
            .format(lang)).add('language', lang)


def createSpeechRec(model):
    speechRec = gizmo.SpeechRecognition()

    if 'vosk' in model:
        modelPath = model['vosk'].get('model-path', '')
        if modelPath and not os.path.isabs(modelPath):
            basePath = model.get('_basePath', '')
            modelPath = os.path.join(basePath, modelPath)
        speechRec.setParam('model-path', modelPath)

        sampleRate = model['vosk'].get('sample-rate', '16000')
        speechRec.setParam('-frate', str(sampleRate))

        for key, val in model['vosk'].items():
            if key not in ('model-path', 'sample-rate'):
                speechRec.setParam(key, str(val))
    elif 'sphinx' in model:
        for key, val in model['sphinx'].items():
            speechRec.setParam(key, val)
    else:
        logger.warning('speech model has no engine configuration, '
                       'attempting Vosk with default params')
        if 'path' in model:
            speechRec.setParam('model-path', model['path'])

    return speechRec


def getSpeechAudioFormat(speechModel):
    try:
        sampleFormat = getattr(gizmo.AVSampleFormat,
                speechModel.get('sampleformat', 'S16'))

        sampleRate = speechModel.get('samplerate', 16000)
        if isinstance(sampleRate, str):
            sampleRate = int(sampleRate)

        return gizmo.AudioFormat(sampleFormat, sampleRate, 1)
    except Exception:
        raise error.Error(_('Invalid speech audio format'))

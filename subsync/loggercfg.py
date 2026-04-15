import logging
import threading
import sys
import gizmo


class BlacklistFilter(logging.Filter):
    def __init__(self, names):
        super().__init__()
        self.blacklist = set(names)

    def filter(self, record):
        if record.name in self.blacklist:
            return False

        try:
            pos = -1
            while True:
                pos = record.name.index('.', pos+1)
                if record.name[:pos] in self.blacklist:
                    return False
        except ValueError:
            return True


initialized = False
_activeFilter = None


def init(level=None, path=None):
    logging.captureWarnings(True)
    numLevel = parseLevel(level)

    for handler in logging.root.handlers[:]:
        logging.root.removeHandler(handler)

    config = dict(
            format='%(asctime)s.%(msecs)03i: %(threadName)12.12s: %(levelname)8.8s: %(name)26s: %(message)s',
            datefmt='%H:%M:%S',
            level=numLevel)

    try:
        logging.basicConfig(**config, filename=path)
    except Exception:
        logging.basicConfig(**config)
        logging.getLogger().error("invalid log file path '%s', ignoring", path, exc_info=True)

    def excepthook(type, exc, tb):
        logging.getLogger().critical("Unhandled exception", exc_info=(type, exc, tb))
        sys.__excepthook__(type, exc, tb)

    sys.excepthook = excepthook

    def thread_excepthook(args):
        logging.getLogger().critical(
            "Unhandled exception in thread %s",
            args.thread.name if args.thread else "unknown",
            exc_info=(args.exc_type, args.exc_value, args.exc_traceback))

    threading.excepthook = thread_excepthook

    gizmo.setDebugLevel(numLevel)

    global initialized
    initialized = True


def setLevel(level):
    numLevel = parseLevel(level)
    logger = logging.getLogger()
    logger.setLevel(numLevel)
    for handler in logger.handlers:
        handler.setLevel(numLevel)
    gizmo.setDebugLevel(numLevel)


def parseLevel(level):
    try:
        return int(level)
    except (TypeError, ValueError):
        try:
            return getattr(logging, level)
        except (TypeError, AttributeError):
            return logging.WARNING


def setBlacklistFilters(filters):
    global _activeFilter

    if _activeFilter:
        for handler in logging.root.handlers:
            handler.removeFilter(_activeFilter)

        _activeFilter = None

    if filters:
        _activeFilter = BlacklistFilter(filters)

        for handler in logging.root.handlers:
            handler.addFilter(_activeFilter)

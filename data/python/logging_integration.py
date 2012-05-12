import clementine
import logging

def setup_logging():
  class ClementineLogHandler(logging.Handler):
    def emit(self, record):
      clementine.logging_handler(
        record.levelno, record.name, record.lineno, record.getMessage())

  root = logging.getLogger()
  root.addHandler(ClementineLogHandler())
  root.setLevel(logging.DEBUG)


setup_logging()

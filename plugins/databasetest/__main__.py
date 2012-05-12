import clementine
import clementine.models
import logging

LOGGER = logging.getLogger("databasetest")


class Plugin(object):
  def __init__(self, app):
    self.app = app

    session = self.app.database.session()

    LOGGER.info("First 10 songs:")
    for song in session.query(clementine.models.Song)[:10]:
      LOGGER.info("'%s' by '%s'", song.title, song.artist)

import clementine
import clementine.models
import logging

LOGGER = logging.getLogger("databasetest")


class DatabaseDelegate(clementine.DatabaseDelegate):
  def directory_added(self, path):
    LOGGER.info("Directory added: %s", path)

  def directory_removed(self, path):
    LOGGER.info("Directory removed: %s", path)

  def songs_changed(self, songs):
    LOGGER.info("%d songs changed:", len(songs))
    for song in songs:
      LOGGER.info("'%s' by '%s'", song.title, song.artist)

  def songs_removed(self, songs):
    LOGGER.info("%d songs removed:", len(songs))
    for song in songs:
      LOGGER.info("'%s' by '%s'", song.title, song.artist)

  def total_song_count_updated(self, total):
    LOGGER.info("Total song count is now %d", total)


class Plugin(object):
  def __init__(self, app):
    self.app = app
    self.app.database.register_delegate(DatabaseDelegate())

    session = self.app.database.session()

    LOGGER.info("First 10 songs:")
    for song in session.query(clementine.models.Song)[:10]:
      LOGGER.info("'%s' by '%s'", song.title, song.artist)

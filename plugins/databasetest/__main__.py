import clementine
import clementine.models
import logging

import sqlalchemy

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


class RandomSongAction(clementine.Action):
  def __init__(self, app):
    super(RandomSongAction, self).__init__("Print a random song")

    self.app = app

  def triggered(self):
    session = self.app.database.session()
    query = session.query(clementine.models.Song)
    query = query.order_by(sqlalchemy.func.random())
    query = query.limit(1)

    song = query.one()

    LOGGER.info("Your random song is '%s' by '%s'", song.title, song.artist)


class Plugin(object):
  def __init__(self, app):
    self.app = app

    # Register for DB changes
    self.app.database.register_delegate(DatabaseDelegate())

    # Print the first 10 songs in the DB
    session = self.app.database.session()

    LOGGER.info("First 10 songs:")
    for song in session.query(clementine.models.Song)[:10]:
      LOGGER.info("'%s' by '%s'", song.title, song.artist)

    # Add a menu item
    self.app.user_interface.add_menu_item("extras", RandomSongAction(app))

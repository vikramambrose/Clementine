import clementine
import clementine.models
import logging

LOGGER = logging.getLogger("testplugin")


class PlayerDelegate(clementine.PlayerDelegate):
  def state_changed(self, state):
    LOGGER.info("State %s", state)

  def volume_changed(self, volume):
    LOGGER.info("Volume %d", volume)

  def position_changed(self, position):
    LOGGER.info("Seeked to %d", position)

  def song_changed(self, song):
    LOGGER.info("Song changed to '%s' by '%s'", song.title, song.artist)


class Plugin(object):
  def __init__(self, app):
    LOGGER.debug("Initialising plugin")

    self.app = app
    self.app.player.register_delegate(PlayerDelegate())

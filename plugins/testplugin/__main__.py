import clementine
import clementine.models
import logging

LOGGER = logging.getLogger("testplugin")


class PlayerDelegate(clementine.PlayerDelegate):
  def state_changed(self, state):
    print "State", state

  def volume_changed(self, volume):
    print "Volume", volume

  def position_changed(self, position):
    print "Seeked to", position

  def song_changed(self, song):
    print "Song changed to '%s' by '%s'" % (song.title, song.artist)


class Plugin(object):
  def __init__(self, app):
    LOGGER.debug("Initialising plugin")

    self.app = app
    self.app.player.register_delegate(PlayerDelegate())

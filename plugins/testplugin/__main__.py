import clementine

class PlayerDelegate(clementine.PlayerDelegate):
  def state_changed(self, state):
    print "State", state

  def volume_changed(self, volume):
    print "Volume", volume

  def position_changed(self, position):
    print "Seeked to", position


class Plugin(object):
  def __init__(self, app):
    self.app = app
    self.app.player.register_delegate(PlayerDelegate())
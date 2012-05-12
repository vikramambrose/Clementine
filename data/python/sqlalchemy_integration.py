import clementine
import logging
import sqlalchemy
import sqlalchemy.orm

logging.getLogger('sqlalchemy.engine').setLevel(logging.INFO)

class Injector(object):
  class __metaclass__(clementine.Database.__class__):
    def __init__(self, name, bases, dict):
      for b in bases:
        if type(b) not in (self, type):
          for k,v in dict.items():
            setattr(b,k,v)
      return type.__init__(self, name, bases, dict)


class Database(Injector, clementine.Database):
  def session(self):
    if not hasattr(self, "_SessionMaker"):
      engine = sqlalchemy.create_engine(self.database_url)
      self._SessionMaker = sqlalchemy.orm.sessionmaker(bind=engine)

    return self._SessionMaker()


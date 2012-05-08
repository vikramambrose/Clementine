from sqlalchemy import Table, Column, Integer, String, MetaData, ForeignKey, Float, Boolean
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import relationship, backref


Base = declarative_base()


class Directory(Base):
  __tablename__ = "directories"

  id   = Column("ROWID", Integer, primary_key=True)
  path = Column(String)
  
  songs          = relationship("Song", backref="directory")
  subdirectories = relationship("Subdirectory", backref="directory")


class Subdirectory(Base):
  __tablename__ = "subdirectories"
  
  id           = Column("ROWID", Integer, primary_key=True)
  directory_id = Column("directory", Integer, ForeignKey("directories.ROWID"))
  path         = Column(String)
  mtime        = Column(Integer)
  
  directory    = backref("directories")


class Song(Base):
  __tablename__ = "songs"
  
  id            = Column("ROWID", Integer, primary_key=True)
  directory_id  = Column("directory", Integer, ForeignKey("directories.ROWID"))
  
  directory     = backref("directories")
  
  # Metadata read from file tags
  title       = Column(String)
  artist      = Column(String)
  album       = Column(String)
  albumartist = Column(String)
  composer    = Column(String)
  track       = Column(Integer)
  disc        = Column(Integer)
  bpm         = Column(Float)
  year        = Column(Integer)
  genre       = Column(String)
  comment     = Column(Integer)
  compilation = Column(Boolean)
  length      = Column(Integer)
  bitrate     = Column(Integer)
  samplerate  = Column(Integer)
  sampler     = Column(Integer)
  
  # Metadata about the file itself
  url         = Column("filename", String)
  unavailable = Column(Boolean)
  mtime       = Column(Integer)
  ctime       = Column(Integer)
  filesize    = Column(Integer)
  filetype    = Column(Integer)
  
  # Raw album art information
  art_automatic = Column(String)
  art_manual    = Column(String)
  
  # Statistics
  playcount  = Column(Integer)
  skipcount  = Column(Integer)
  lastplayed = Column(Integer)
  score      = Column(Integer)
  rating     = Column(Integer)
  
  # User preferences
  forced_compilation_on  = Column(Boolean)
  forced_compilation_off = Column(Boolean)
  
  effective_compilation  = Column(Boolean)
  
  # Cue sheet information
  beginning = Column(Integer)
  cue_path  = Column(String)

/* This file is part of Clementine.
   Copyright 2012, David Sansome <me@davidsansome.com>
   
   Clementine is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   Clementine is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with Clementine.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "python_utils.h"
#include "core/logging.h"
#include "core/song.h"

#include <QFile>
#include <QUrl>

namespace python_utils {

object AddModule(const QByteArray& source, const QString& filename,
                 const QString& module_name) {
  object ret;

  try {
    // Create the Python code object
    object code(handle<>(Py_CompileString(
          source.constData(), filename.toUtf8().constData(), Py_file_input)));

    // Create a module for the code object
    ret = object(handle<>(PyImport_ExecCodeModule(
        const_cast<char*>(module_name.toUtf8().constData()), code.ptr())));
  } catch (error_already_set&) {
    PyErr_Print();
    PyErr_Clear();
    return ret;
  }

  // Add the module to a parent module if one exists
  const int dot_index = module_name.lastIndexOf('.');
  if (dot_index != -1) {
    const QString parent_module_name = module_name.left(dot_index);
    try {
      object parent_module(import(parent_module_name.toUtf8().constData()));
      parent_module.attr(module_name.mid(dot_index + 1).toUtf8().constData()) = ret;
    } catch (error_already_set&) {
      PyErr_Print();
      PyErr_Clear();
    }
  }

  return ret;
}

object AddModule(const QString& filename, const QString& module_name) {
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    return object();
  }

  return AddModule(file.readAll(), filename, module_name);
}


struct SongConverter {
  static PyObject* convert(const Song& song) {
    // Create a dict containing the fields in this Song.
    dict kwargs;
    kwargs["id"] = song.id();
    kwargs["directory_id"] = song.directory_id();
    kwargs["title"] = song.title();
    kwargs["artist"] = song.artist();
    kwargs["album"] = song.album();
    kwargs["albumartist"] = song.albumartist();
    kwargs["composer"] = song.composer();
    kwargs["track"] = song.track();
    kwargs["disc"] = song.disc();
    kwargs["bpm"] = song.bpm();
    kwargs["year"] = song.year();
    kwargs["genre"] = song.genre();
    kwargs["comment"] = song.comment();
    kwargs["compilation"] = song.is_compilation();
    kwargs["length"] = song.length_nanosec();
    kwargs["bitrate"] = song.bitrate();
    kwargs["samplerate"] = song.samplerate();
    kwargs["url"] = song.url().toString();
    kwargs["unavailable"] = song.is_unavailable();
    kwargs["mtime"] = song.mtime();
    kwargs["ctime"] = song.ctime();
    kwargs["filesize"] = song.filesize();
    kwargs["filetype"] = int(song.filetype());
    kwargs["art_automatic"] = song.art_automatic();
    kwargs["art_manual"] = song.art_manual();
    kwargs["playcount"] = song.playcount();
    kwargs["skipcount"] = song.skipcount();
    kwargs["lastplayed"] = song.lastplayed();
    kwargs["score"] = song.score();
    kwargs["rating"] = song.rating();
    kwargs["cue_path"] = song.cue_path();

    // Get the Python Song class.
    object py_song_class(import("clementine.models").attr("Song"));

    // Construct one as a copy of this Song.
    return incref(py_song_class(*tuple(), **kwargs).ptr());
  }
};

struct QStringConverter {
  static PyObject* convert(const QString& s) {
    const QByteArray utf8(s.toUtf8());
    return incref(PyUnicode_FromStringAndSize(utf8.constData(), utf8.length()));
  }

  static void* convertible(PyObject* obj) {
    if (!PyString_Check(obj) && !PyUnicode_Check(obj))
      return NULL;
    return obj;
  }

  static void construct(PyObject* obj, boost::python::converter::rvalue_from_python_stage1_data* data) {
    void* storage = ((boost::python::converter::rvalue_from_python_storage<QString>*)data)->storage.bytes;

    if (PyString_Check(obj)) {
      new (storage) QString(PyString_AsString(obj));
    } else {
#if Py_UNICODE_SIZE == 4
      new (storage) QString(QString::fromUcs4(PyUnicode_AS_UNICODE(obj), PyUnicode_GET_SIZE(obj)));
#else
      new (storage) QString(QString::fromUtf16(PyUnicode_AS_UNICODE(obj), PyUnicode_GET_SIZE(obj)));
#endif
    }

    data->convertible = storage;
  }
};

template <typename T>
struct QListConverter {
  static PyObject* convert(const QList<T>& l) {
    list ret;
    foreach (const T& item, l) {
      ret.append(item);
    }
    return incref(ret.ptr());
  }
};

void RegisterTypeConverters() {
  to_python_converter<QString, QStringConverter>();
  to_python_converter<Song, SongConverter>();

  to_python_converter<QList<QString>, QListConverter<QString> >();
  to_python_converter<QList<Song>, QListConverter<Song> >();

  converter::registry::push_back(QStringConverter::convertible,
                                 QStringConverter::construct,
                                 type_id<QString>());
}

}

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

#include "availableplugin.h"
#include "pythonlanguage.h"
#include "core/logging.h"
#include "core/song.h"

#include <clementine/Clementine>
#include <clementine/Player>
#include <clementine/PlayerDelegate>
#include <clementine/Plugin>

#include <QFile>
#include <QStringList>
#include <QUrl>

#include <boost/python.hpp>
#include <Python.h>

using namespace boost::python;

namespace {

object AddModule(const QByteArray& source, const QByteArray& filename,
                 const QString& module_name) {
  object ret;

  try {
    // Create the Python code object
    object code(handle<>(Py_CompileString(
          source.constData(), filename.constData(), Py_file_input)));

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

  return AddModule(file.readAll(), filename.toUtf8(), module_name);
}

}

#define WRAPPER_FUNCTION(cpp_name, python_name, arg_types, arg_names) \
  void cpp_name arg_types { \
    if (override f = this->get_override(#python_name)) { \
      f arg_names ; \
    } else { \
      Default##cpp_name arg_names ; \
    } \
  } \
  void Default##cpp_name arg_types  { \
    this->PlayerDelegate::cpp_name arg_names ; \
  }


struct PlayerDelegateWrapper : clementine::PlayerDelegate,
                               wrapper<clementine::PlayerDelegate> {
  WRAPPER_FUNCTION(StateChanged, state_changed, (clementine::Player::State state), (state))
  WRAPPER_FUNCTION(VolumeChanged, volume_changed, (int percent), (percent))
  WRAPPER_FUNCTION(PositionChanged, position_changed, (int64_t microseconds), (microseconds))
  WRAPPER_FUNCTION(PlaylistFinished, playlist_finished, (), ())
  WRAPPER_FUNCTION(SongChanged, song_changed, (const Song& song), (song))
};


class PythonPlugin : public clementine::Plugin {
public:
  PythonPlugin(object py_object, clementine::Clementine* clem,
               const clementine::AvailablePlugin& plugin_info)
    : Plugin(clem, plugin_info),
      py_object_(py_object)
  {
  }

private:
  object py_object_;
};


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
};


BOOST_PYTHON_MODULE(clementine) {
  class_<clementine::Clementine, boost::noncopyable>("Clementine", no_init)
      .add_property("player",
                    make_function(&clementine::Clementine::player,
                                  return_value_policy<reference_existing_object>()));

  {
    scope s = class_<clementine::Player, boost::noncopyable>("Player", no_init)
        .add_property("volume_percent",
                      &clementine::Player::GetVolumePercent,
                      &clementine::Player::SetVolumePercent)
        .add_property("position_seconds",
                      &clementine::Player::GetPositionSeconds,
                      &clementine::Player::SeekToSeconds)
        .add_property("position_nanoseconds",
                      &clementine::Player::GetPositionNanoseconds,
                      &clementine::Player::SeekToNanoseconds)
        .add_property("state",
                      &clementine::Player::GetState,
                      &clementine::Player::SetState)
        .def("play", &clementine::Player::Play)
        .def("pause", &clementine::Player::Pause)
        .def("play_pause", &clementine::Player::PlayPause)
        .def("stop", &clementine::Player::Stop)
        .def("next", &clementine::Player::Next)
        .def("previous", &clementine::Player::Previous)
        .def("toggle_mute", &clementine::Player::ToggleMute)
        .def("show_osd", &clementine::Player::ShowOSD)
        .def("register_delegate", &clementine::Player::RegisterDelegate)
        .def("unregister_delegate", &clementine::Player::UnregisterDelegate);

    enum_<clementine::Player::State>("State")
        .value("STOPPED", clementine::Player::State_Stopped)
        .value("PLAYING", clementine::Player::State_Playing)
        .value("PAUSED", clementine::Player::State_Paused);
  }

  class_<PlayerDelegateWrapper, boost::noncopyable>("PlayerDelegate")
      .def("state_changed",
           &clementine::PlayerDelegate::StateChanged,
           &PlayerDelegateWrapper::DefaultStateChanged)
      .def("volume_changed",
           &clementine::PlayerDelegate::VolumeChanged,
           &PlayerDelegateWrapper::DefaultVolumeChanged)
      .def("position_changed",
           &clementine::PlayerDelegate::PositionChanged,
           &PlayerDelegateWrapper::DefaultPositionChanged)
      .def("playlist_finished",
           &clementine::PlayerDelegate::PlaylistFinished,
           &PlayerDelegateWrapper::DefaultPlaylistFinished)
      .def("song_changed",
           &clementine::PlayerDelegate::SongChanged,
           &PlayerDelegateWrapper::DefaultSongChanged);
}


PythonLanguage::PythonLanguage(clementine::Clementine* clem)
  : clementine::Language(clem) {
}

bool PythonLanguage::Init() {
  Py_Initialize();
  initclementine();

  to_python_converter<QString, QStringConverter>();
  to_python_converter<Song, SongConverter>();

  if (!AddModule(":python/models.py", "clementine.models"))
    return false;

  return true;
}

clementine::Plugin* PythonLanguage::LoadPlugin(const clementine::AvailablePlugin& plugin) {
  // Open the source file
  const QString filename = plugin.path_ + "/__main__.py";
  QFile file(filename);
  if (!file.open(QIODevice::ReadOnly)) {
    qLog(Warning) << "Couldn't open Python file" << filename;
    return NULL;
  }

  // Read its contents
  const QByteArray source = file.readAll();

  object module(AddModule(source, filename.toUtf8(),
                          QString("clementineplugin_" + plugin.id_).toUtf8()));
  if (!module) {
    return NULL;
  }

  // Get the plugin's Plugin class.
  object plugin_class;
  try {
    plugin_class = module.attr("Plugin");
  } catch (error_already_set&) {
    if (PyErr_ExceptionMatches(PyExc_AttributeError)) {
      qLog(Warning) << plugin.id_
                    << ": You must define a class called 'Plugin' in __main__.py";
    } else {
      PyErr_Print();
    }
    PyErr_Clear();
    return NULL;
  }

  // Try to instantiate it.
  object py_object;
  try {
    object py_app(handle<>(
        reference_existing_object::apply<clementine::Clementine*>::type()(clementine_)));
    py_object = plugin_class(py_app);
  } catch (error_already_set&) {
    PyErr_Print();
    PyErr_Clear();
    return NULL;
  }

  // Create and return the C++ plugin.
  return new PythonPlugin(py_object, clementine_, plugin);
}

void PythonLanguage::UnloadPlugin(clementine::Plugin* plugin) {
}

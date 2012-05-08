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

#ifndef PYTHON_WRAPPERS_H
#define PYTHON_WRAPPERS_H

#include "core/song.h"

#include <clementine/PlayerDelegate>

#include <boost/python.hpp>

// This header contains concrete implementations of abstract base classes that
// call functions in Python children.
// The WRAPPER_FUNCTION macro is nasty but hides some of the copy+paste.

#define WRAPPER_FUNCTION(class_name, cpp_name, python_name, arg_types, arg_names) \
  void cpp_name arg_types { \
    if (override f = this->get_override(#python_name)) { \
      f arg_names ; \
    } else { \
      Default##cpp_name arg_names ; \
    } \
  } \
  void Default##cpp_name arg_types  { \
    this->class_name::cpp_name arg_names ; \
  }


struct PlayerDelegateWrapper : clementine::PlayerDelegate,
                               boost::python::wrapper<clementine::PlayerDelegate> {
  WRAPPER_FUNCTION(PlayerDelegateWrapper, StateChanged, state_changed, (clementine::Player::State state), (state))
  WRAPPER_FUNCTION(PlayerDelegateWrapper, VolumeChanged, volume_changed, (int percent), (percent))
  WRAPPER_FUNCTION(PlayerDelegateWrapper, PositionChanged, position_changed, (int64_t microseconds), (microseconds))
  WRAPPER_FUNCTION(PlayerDelegateWrapper, PlaylistFinished, playlist_finished, (), ())
  WRAPPER_FUNCTION(PlayerDelegateWrapper, SongChanged, song_changed, (const Song& song), (song))
};


#undef WRAPPER_FUNCTION

#endif // PYTHON_WRAPPERS_H

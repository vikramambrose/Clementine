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

#ifndef PYTHONLANGUAGE_H
#define PYTHONLANGUAGE_H

#include "language.h"

#include <QMap>

#include <boost/python.hpp>

class PythonLanguage : public clementine::Language {
public:
  PythonLanguage(clementine::Clementine* clem);

  QString name() const { return "python"; }

  virtual clementine::Plugin* LoadPlugin(const clementine::AvailablePlugin& plugin);
  virtual void UnloadPlugin(clementine::Plugin* plugin);

protected:
  virtual bool Init();

private:
  QMap<QString, boost::python::object> loaded_plugins_;
};

#endif // PYTHONLANGUAGE_H

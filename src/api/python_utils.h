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

#ifndef PYTHON_UTILS_H
#define PYTHON_UTILS_H

#include <boost/python.hpp>
#include <Python.h>

class QByteArray;
class QString;

using namespace boost::python;

namespace python_utils {

// Compiles the source to a codeobject and adds it to the interpreter.  The
// filename is used for the new module's __file__ attribute.
object AddModule(const QByteArray& source, const QString& filename,
                 const QString& module_name);

// Same as above, except the module's source is loaded by QFile.
object AddModule(const QString& filename, const QString& module_name);

// Registers all our custom boost::python type converters.  Must be called
// after Py_Initialise, but before anything else.
void RegisterTypeConverters();

} // namespace python_utils


#endif // PYTHON_UTILS_H

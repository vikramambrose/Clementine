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

#include <clementine/Clementine>
#include <clementine/Player>

class Application;

namespace clementine {

struct Clementine::Private {
  Application* app_;

  boost::shared_ptr<Player> player_;
};

Clementine::Clementine(void* app)
  : d(new Private)
{
  d->app_ = reinterpret_cast<Application*>(app);
  d->player_.reset(new Player(app));
}

Clementine::~Clementine() {
}

boost::shared_ptr<Player> Clementine::player() const {
  return d->player_;
}

void Clementine::UnregisterAllDelegates() {
  d->player_->UnregisterAllDelegates();
}

} // namespace clementine

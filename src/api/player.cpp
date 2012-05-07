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

#include "player_p.h"
#include "core/application.h"
#include "core/player.h"
#include "core/timeconstants.h"
#include "engines/enginebase.h"

#include <clementine/Player>

#include <QList>

namespace clementine {

Player::Player(void* app)
  : d(new PlayerPrivate)
{
  d->app_ = reinterpret_cast<Application*>(app);
  d->listener_ = new PlayerListener(d.get());
}

Player::~Player() {
  delete d->listener_;
}

int Player::GetVolumePercent() const {
  return d->app_->player()->GetVolume();
}

int Player::GetPositionSeconds() const {
  return GetPositionNanoseconds() / kNsecPerSec;
}

int64_t Player::GetPositionNanoseconds() const {
  return d->app_->player()->engine()->position_nanosec();
}

Player::State Player::GetState() const {
  switch (d->app_->player()->GetState()) {
  case Engine::Playing:
    return State_Playing;

  case Engine::Paused:
    return State_Paused;

  case Engine::Empty:
  case Engine::Idle:
  default:
    return State_Stopped;
  }
}

void Player::SetState(State state) {
  switch (state) {
  case State_Stopped:
    Stop();
    break;

  case State_Playing:
    Play();
    break;

  case State_Paused:
    Pause();
    break;
  }
}

void Player::Play() {
  d->app_->player()->Play();
}

void Player::Pause() {
  d->app_->player()->Pause();
}

void Player::PlayPause() {
  d->app_->player()->PlayPause();
}

void Player::Stop() {
  d->app_->player()->Stop();
}

void Player::Next() {
  d->app_->player()->Next();
}

void Player::Previous() {
  d->app_->player()->Previous();
}

void Player::SetVolumePercent(int percentage) {
  d->app_->player()->SetVolume(percentage);
}

void Player::ToggleMute() {
  d->app_->player()->Mute();
}

void Player::SeekToSeconds(int seconds) {
  d->app_->player()->SeekTo(seconds);
}

void Player::SeekToNanoseconds(int64_t nanonseconds) {
  SeekToSeconds(nanonseconds / kNsecPerSec);
}

void Player::ShowOSD() {
  d->app_->player()->ShowOSD();
}

void Player::RegisterDelegate(PlayerDelegatePtr delegate) {
  d->delegates_.append(delegate);
}

void Player::UnregisterDelegate(PlayerDelegatePtr delegate) {
  d->delegates_.removeAll(delegate);
}


PlayerListener::PlayerListener(PlayerPrivate* _d, QObject* parent)
  : QObject(parent),
    d(_d)
{
  connect(d->app_->player(), SIGNAL(Playing()), SLOT(Playing()));
  connect(d->app_->player(), SIGNAL(Paused()), SLOT(Paused()));
  connect(d->app_->player(), SIGNAL(Stopped()), SLOT(Stopped()));
  connect(d->app_->player(), SIGNAL(PlaylistFinished()), SLOT(PlaylistFinished()));
  connect(d->app_->player(), SIGNAL(VolumeChanged(int)), SLOT(VolumeChanged(int)));
  connect(d->app_->player(), SIGNAL(Seeked(qlonglong)), SLOT(Seeked(qlonglong)));
}

void PlayerListener::Playing() {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->StateChanged(Player::State_Playing);
  }
}

void PlayerListener::Paused() {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->StateChanged(Player::State_Paused);
  }
}

void PlayerListener::Stopped() {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->StateChanged(Player::State_Stopped);
  }
}

void PlayerListener::PlaylistFinished() {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->PlaylistFinished();
  }
}

void PlayerListener::VolumeChanged(int volume) {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->VolumeChanged(volume);
  }
}

void PlayerListener::Seeked(qlonglong microseconds) {
  foreach (PlayerDelegatePtr delegate, d->delegates_) {
    delegate->PositionChanged(microseconds);
  }
}

} // namespace clementine

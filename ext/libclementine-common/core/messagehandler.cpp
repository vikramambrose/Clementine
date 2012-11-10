/* This file is part of Clementine.
   Copyright 2011, David Sansome <me@davidsansome.com>

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

// Note: this file is licensed under the Apache License instead of GPL because
// it is used by the Spotify blob which links against libspotify and is not GPL
// compatible.


#include "messagehandler.h"
#include "core/logging.h"

#include <QAbstractSocket>
#include <QLocalSocket>

_MessageHandlerBase::_MessageHandlerBase(QIODevice* device, QObject* parent)
  : QObject(parent),
    device_(NULL),
    flush_abstract_socket_(NULL),
    flush_local_socket_(NULL),
    is_device_closed_(false) {
  if (device) {
    SetDevice(device);
  }
}

void _MessageHandlerBase::SetDevice(QIODevice* device) {
  device_ = device;

  connect(device, SIGNAL(readyRead()), SLOT(DeviceReadyRead()));
  connect(device, SIGNAL(bytesWritten(qint64)), SLOT(DeviceBytesWritten(qint64)));

  // Yeah I know.
  if (QAbstractSocket* socket = qobject_cast<QAbstractSocket*>(device)) {
    flush_abstract_socket_ = &QAbstractSocket::flush;
    connect(socket, SIGNAL(disconnected()), SLOT(DeviceClosed()));
  } else if (QLocalSocket* socket = qobject_cast<QLocalSocket*>(device)) {
    flush_local_socket_ = &QLocalSocket::flush;
    connect(socket, SIGNAL(disconnected()), SLOT(DeviceClosed()));
  } else {
    qFatal("Unsupported device type passed to _MessageHandlerBase");
  }
}

void _MessageHandlerBase::DeviceReadyRead() {
  while (device_->bytesAvailable()) {
    qLog(Debug) << "Device has" << device_->bytesAvailable() << "bytes ready to read";

    QDataStream s(device_);
    char* data_pointer = 0;
    uint data_length = 0;
    s.readBytes(data_pointer, data_length);

    QByteArray data = QByteArray::fromRawData(data_pointer, data_length);
    qLog(Debug) << "Read" << data.length() << "bytes:" << data.toHex();

    // Parse the message
    if (!RawMessageArrived(data)) {
      qLog(Error) << "Malformed protobuf message";
      delete[] data_pointer;
      device_->close();
      return;
    }

    // Clear the buffer
    qLog(Debug) << "Ready to accept next protobuf";
    delete[] data_pointer;
  }
}

void _MessageHandlerBase::WriteMessage(const QByteArray& data) {
  qLog(Debug) << "Writing message" << data.toHex() << "(length" << data.length() << ")";
  qLog(Debug) << "Bytes to write" << device_->bytesToWrite();

  QDataStream s(device_);
  s.writeBytes(data.constData(), data.length());

  qLog(Debug) << "Bytes to write" << device_->bytesToWrite();

  // Sorry.
  if (flush_abstract_socket_) {
    qLog(Debug) << "Flushing abstract socket";
    ((static_cast<QAbstractSocket*>(device_))->*(flush_abstract_socket_))();
  } else if (flush_local_socket_) {
    qLog(Debug) << "Flushing local socket";
    ((static_cast<QLocalSocket*>(device_))->*(flush_local_socket_))();
  }
  qLog(Debug) << "Bytes to write" << device_->bytesToWrite();
}

void _MessageHandlerBase::DeviceBytesWritten(qint64 bytes) {
  qLog(Debug) << "Written" << bytes << "bytes";
}

void _MessageHandlerBase::DeviceClosed() {
  qLog(Debug) << "Device closed";
  is_device_closed_ = true;
  AbortAll();
}

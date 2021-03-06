/**
 * Copyright (c) 2012, Thibault Signor <tibsou@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE REGENTS AND CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "Connection.h"
#include "Protocol.h"

Connection::Connection() {
  users = new QMap<quint16, User *>;

  socket = new QTcpSocket();
  connect(socket, SIGNAL(readyRead()), this, SLOT(dataRecv()));
  connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this,
          SLOT(socketError(QAbstractSocket::SocketError)));

  heartbeat = new QTimer;
  connect(heartbeat, SIGNAL(timeout()), this, SLOT(sendHeartbeat()));
  message = new QString;
  messageSize = 0;
}

void Connection::sendPacket(const QByteArray &packet) {
  if (socket->state() == QTcpSocket::ConnectedState) {
    socket->write(packet);
    heartbeat->stop();
    heartbeat->start(HEARTBEAT_RATE);
  }
}

void Connection::dataSend(quint16 msgCode, QString msgToSend) {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);

  out << static_cast<quint16>(0);
  out << static_cast<quint16>(msgCode);
  out << msgToSend;
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));

  sendPacket(packet);
}

void Connection::sendHeartbeat() {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);

  out << static_cast<quint16>(0);
  out << static_cast<quint16>(CS_HEARTBEAT);
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));

  sendPacket(packet);
}

void Connection::positionSend(float x, float y, float z, float pitch,
                              float yaw) {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);

  out << static_cast<quint16>(0);
  out << static_cast<quint16>(CS_AVATAR_POSITION);
  out << x;
  out << y;
  out << z;
  out << pitch;
  out << yaw;
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));

  sendPacket(packet);
}

void Connection::dataRecv() {
  forever {
    QDataStream in(socket);
    if (messageSize == 0) {
      if (socket->bytesAvailable() < static_cast<int>(sizeof(quint16))) return;
      in >> messageSize;
    }

    if (socket->bytesAvailable() < messageSize)
      return;  // we wait for the end of the message

    quint16 messageCode;
    in >> messageCode;

    if (messageCode < 0x10) {
      QString messageText;
      in >> messageText;

      dataHandler(messageCode, messageText);
    } else {
      quint16 id;
      float x, y, z, pitch, yaw;
      in >> id;
      in >> x;
      in >> y;
      in >> z;
      in >> pitch;
      in >> yaw;
      if (users->contains(id)) {
        User *u = users->value(id);
        u->setPosition(x, y, z, pitch, yaw);
        emit userPosition(u);
      }
    }
    // we reset the messageSize to 0, waiting for next data
    messageSize = 0;
  }
}

void Connection::dataHandler(quint16 dataCode, QString data) {
  QStringList split;
  QString sender;
  QList<quint16> usersInList;

  switch (dataCode) {
    case SC_MSG_PUBLIC:
      split = data.split(":");
      sender = split[0];
      split.removeAt(0);
      data = sender + ": " + split.join(":");
      displayData(data, dataCode);
      break;
    case SC_ER_NICKINUSE:
      data = tr("This nick is already used. Please retry with another one");
      displayData(data, dataCode);
      break;
    case SC_ER_ERRONEOUSNICK:
      data = tr("Erroneous nickname. Please retry with another one");
      displayData(data, dataCode);
      break;
    case SC_MSG_EVENT:
      displayData(data, dataCode);
      break;
    case SC_USER_JOIN:
      dataSend(CS_USER_LIST);
      data = data + tr(" just logged in");
      displayData(data, dataCode);
      break;
    case SC_USER_PART:
      dataSend(CS_USER_LIST);
      data = data + tr(" has quit");
      displayData(data, dataCode);
      break;
    case SC_USER_LIST:
      if (!data.isEmpty()) {
        foreach (QString pair, data.split(";")) {
          quint16 newId = pair.split(":").at(0).toUShort();
          QString newNickname = pair.split(":").at(1);

          usersInList.append(newId);

          /* NOTE TEMPORAIRE:
           - un user peut parfaitement changer d'id lorsqu'on est déconnecté
           - cette méthode garantit l'unicité des ids, mais pas celle des nicks
           Il faut donc détruire systématiquement la liste des users lorsqu'on
           est déconnecté et qu'elle se reconstruise dans le bon ordre sur
           chaque événement (ce qui ne devrait pas poser de problème a priori,
           vu la structure actuelle du code)
           */

          if (!users->contains(newId)) {  // Creating user
            User *newUser = new User(newId, newNickname);
            emit userCreated(newUser);
            users->insert(newId, newUser);
          }
        }
      }
      foreach (quint16 userId, users->keys()) {  // Finding users to delete
        if (!usersInList.contains(userId)) {     // Deleting user
          User *oldUser = users->value(userId);
          emit userDeleted(oldUser);
          delete oldUser;
          users->remove(userId);
        }
      }
      emit listChanged();
      break;
    case SC_MSG_PRIVATE:
      split = data.split(":");
      sender = split[0];
      split.removeAt(0);
      data = sender + ": " + split.join(":");
      displayData(data, dataCode);
      break;
    default:
      displayData(tr("Unknown message received"));
  }
}

void Connection::socketError(QAbstractSocket::SocketError error) {
  switch (error) {
    case QAbstractSocket::HostNotFoundError:
      displayData(tr("ERROR : host not found"));
      break;
    case QAbstractSocket::ConnectionRefusedError:
      displayData(tr("ERROR : connection refused. Is the server launched?"));
      break;
    case QAbstractSocket::RemoteHostClosedError:
      displayData(tr("ERROR : connection closed by remote host"));
      break;
    default:
      displayData(tr("ERROR : ") + socket->errorString());
  }
}

void Connection::displayData(QString data, quint16 type) {
  *message = data;
  messageType = type;
  emit messageChanged();
}

void Connection::clearUserlist() {
  foreach (quint16 id, users->keys()) {
    User *oldUser = users->value(id);
    emit userDeleted(oldUser);
    delete oldUser;
    users->remove(id);
  }
}

QTcpSocket *Connection::getSocket() { return socket; }

QMap<quint16, User *> *Connection::getUsers() { return users; }

quint16 Connection::getIdByNick(QString nick) {
  foreach (quint16 id, users->keys()) {
    if (users->value(id)->getNickname() == nick) return id;
  }
  return 0;
}

QString Connection::getMessage() { return *message; }

quint16 Connection::getMessageType() { return messageType; }

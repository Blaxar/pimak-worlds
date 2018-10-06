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

#include "Client.h"
#include "Protocol.h"
#include "Packets.h"
#include <iostream>

QMap<quint16, Client *> Client::clients;

Client::Client(QTcpSocket *tcp, QObject *p)
    : QObject(p), clientTcp(tcp), messageSize(0) {
  tcp->setParent(this);  // QTcpSocket must be destroyed with Client

  connect(clientTcp, SIGNAL(disconnected()), SLOT(clientDisconnect()));
  connect(clientTcp, SIGNAL(readyRead()), SLOT(dataRecv()));
  lifetime = new QTimer;
  connect(lifetime, SIGNAL(timeout()), SLOT(clientDisconnect()));

  nickname = new QString;
  posChanged = true;
  needUpdate = true;

  for (quint16 i = 1; i <= clients.size() + 1; i++) {
    if (!clients.contains(i)) id = i;
  }
  clients.insert(id, this);  // Adding the client to the list
}

Client::~Client() { clients.remove(id); }

QMap<quint16, Client *> Client::getClients() { return clients; }

void Client::dataRecv() {
  forever {
    QDataStream in(clientTcp);

    std::size_t bytesAvailable = clientTcp->bytesAvailable();
    unpacker.reserve_buffer(bytesAvailable);
    if (bytesAvailable == 0) return;
    std::size_t actualReadSize = in.readRawData(unpacker.buffer(), bytesAvailable);
    unpacker.buffer_consumed(actualReadSize);

    msgpack::object_handle result;
    while(unpacker.next(result)) {

      NetMsg netMsg;
      
      msgpack::object obj(result.get());
      obj.convert(netMsg);

      quint16 &messageCode = netMsg.code;
      NetPayload &payload = netMsg.payload;

      lifetime->stop();
      lifetime->start(CONNECTION_TIMEOUT);

      if (messageCode == CS_AUTH || messageCode == CS_MSG_PUBLIC ||
          messageCode == CS_MSG_PRIVATE || messageCode == CS_USER_LIST) {
        QString message;

        if (nickname->isEmpty())
          std::cout << "[From: " << id << "] (" << messageCode << ") "
                    << message.toStdString() << std::endl;
        else
          std::cout << "[From: " << id << " (" << nickname->toStdString()
                    << ")] (" << messageCode << ") " << message.toStdString()
                    << std::endl;
        dataHandler(messageCode, payload);
      } else if (messageCode == CS_HEARTBEAT) {
        if (nickname->isEmpty())
          std::cout << "[From: " << id << "] (" << messageCode << " - HEARTBEAT)"
                    << std::endl;
        else
          std::cout << "[From: " << id << " (" << nickname->toStdString()
                    << ")] (" << messageCode << " - HEARTBEAT)" << std::endl;
      } else if (messageCode == CS_AVATAR_POSITION) {
      
        float oldX = x;
        float oldY = y;
        float oldZ = z;
        float oldPitch = pitch;
        float oldYaw = yaw;

        msgpack::object_handle oh =
          msgpack::unpack(payload.data(), payload.size());

        msgpack::object deserialized = oh.get();
        deserialized.convert(*this);

        posChanged = !(
          std::abs(x - oldX) <= std::numeric_limits<float>::epsilon() &&
          std::abs(y - oldY) <= std::numeric_limits<float>::epsilon() &&
          std::abs(z - oldZ) <= std::numeric_limits<float>::epsilon() &&
          std::abs(pitch - oldPitch) <= std::numeric_limits<float>::epsilon() &&
          std::abs(yaw - oldYaw) <= std::numeric_limits<float>::epsilon());
        if (posChanged) needUpdate = true;

      } else {
        if (nickname->isEmpty())
          std::cout << "UID " << id << " sent an unknown request!" << std::endl;
        else
          std::cout << "UID " << id << " (" << nickname->toStdString()
                    << ") sent an unknown request!" << std::endl;
      }
    }
  }
}

void Client::dataHandler(quint16 & dataCode, NetPayload & data) {
  QStringList splitted;
  QString receiver;
  bool nickInUse = false;
  msgpack::object_handle oh;
  msgpack::object deserialized;

  if(data.size()>0) {
    oh = msgpack::unpack(data.data(), data.size());
    deserialized = oh.get();
  }

  switch (dataCode) {
    case CS_AUTH: {
      payload::CS::Auth auth;
      deserialized.convert(auth);
      foreach (Client *client, clients) {
        if (client->getNickname() == auth.nickname) {
          nickInUse = true;
          break;
        }
      }
      if (nickInUse) {
        sendTo(id, SC_ER_NICKINUSE);
        emit clientDisconnect();
      } else if (QString::fromStdString(auth.nickname).contains(":")) {
        sendTo(id, SC_ER_ERRONEOUSNICK);
        emit clientDisconnect();
      } else {
        *nickname = QString::fromStdString(auth.nickname);
        sendToAll(SC_USER_JOIN, *nickname);
      }
      break;
    }
    case CS_MSG_PUBLIC: {
      payload::CS::MsgPublic msg_public;
      deserialized.convert(msg_public);
      foreach (quint16 userId, clients.keys()) {
        if (userId != id)
          emit sendTo(userId, SC_MSG_PUBLIC, *nickname + ":" +
                      QString::fromStdString(msg_public.message));
      }
      break;
    }
    case CS_USER_LIST:
      emit sendList();
      break;
    case CS_MSG_PRIVATE: {
      payload::CS::MsgPrivate msg_private;
      deserialized.convert(msg_private);
      // prevent self private messages
      if (msg_private.receiver != id) {
        foreach (Client *client, clients) {
          if (client->getId() == msg_private.receiver) {
            splitted = QStringList(*nickname);  // replace by the sender name
            splitted.append(QString::fromStdString(msg_private.message));
            sendTo(client->getId(), SC_MSG_PRIVATE, splitted.join(":"));
          }
        }
      }
      break;
    }
    default:
      if (nickname->isEmpty())
        std::cout << "UID " << id << " sent an unknown request!" << std::endl;
      else
        std::cout << "UID " << id << " (" << nickname->toStdString()
                  << ") sent an unknown request!" << std::endl;
  }
}

void Client::clientDisconnect() {
  if (!nickname->isEmpty()) emit sendToAll(SC_USER_PART, *nickname);
  // We let the Client delete itself
  deleteLater();
}

void Client::sendPositionToAll() {
  if (needUpdate) {
    foreach (quint16 userId, clients.keys()) {
      if (userId != id) emit sendDataTo(userId, SC_AVATAR_POSITION);
    }
  }

  if (!posChanged) needUpdate = false;
}

void Client::sendList() {
  QStringList userlist;
  foreach (Client *client, clients) {
    if (client->getId() != id)
      userlist.append(QString::number(client->getId()) + ":" +
                      client->getNickname());
  }
  std::cout << "[To: " << id << "] list " << userlist.join(";").toStdString()
            << std::endl;
  emit sendTo(id, SC_USER_LIST, userlist.join(";"));
}

void Client::sendToAll(const quint16 &messageCode, const QString &message) {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);
  out << static_cast<quint16>(0);
  out << static_cast<quint16>(messageCode);
  out << message;
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));

  foreach (Client *client, clients) { client->sendPacket(packet); }
}

void Client::sendTo(quint16 uid, const quint16 &messageCode,
                    const QString &message) {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);
  out << static_cast<quint16>(0);
  out << static_cast<quint16>(messageCode);
  out << message;
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));

  clients[uid]->sendPacket(packet);
  std::cout << "[To: " << uid << "] (" << messageCode << ") "
            << message.toStdString() << std::endl;
}

void Client::sendDataTo(quint16 uid, const quint16 &messageCode) {
  QByteArray packet;
  QDataStream out(&packet, QIODevice::WriteOnly);
  out << static_cast<quint16>(0);
  out << static_cast<quint16>(messageCode);
  out << id;
  out << x;
  out << y;
  out << z;
  out << pitch;
  out << yaw;
  out.device()->seek(0);
  out << static_cast<quint16>(packet.size() - sizeof(quint16));
  clients[uid]->sendPacket(packet);
}

void Client::sendPacket(const QByteArray &packet) {
  clientTcp->write(packet);
  if (clientTcp->state() == QAbstractSocket::ConnectedState)
    clientTcp->waitForBytesWritten(500);
}

QString Client::getNickname() { return *nickname; }

quint16 Client::getId() { return id; }

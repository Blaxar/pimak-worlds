/**
 * Copyright (c) 2018, Thibault Signor         <tibsou@gmail.com>
 *                     Julien 'Blaxar' Bardagi <blaxar.waldarax@gmail.com>
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

#ifndef PACKETS_H
#define PACKETS_H

#include <pwmsgpack.h>
#include <Protocol.h>
#include <Types.h>

class NetMsg {

 public:
  NetMsg(quint16 ver = 0, quint16 c = 0, std::string pl = ""):
    version(ver), code(c), payload(pl) {

  }
  quint16 version;
  quint16 code;
  std::string payload;

  MSGPACK_DEFINE_MAP(version, code, payload);

  template <class T, int V = 0, typename... Arguments>
  static NetMsg make(Arguments... params) {
    T payload(params...);
    std::stringstream buffer;
    msgpack::pack(buffer, payload);
    buffer.seekg(0);
    return NetMsg(V, payload.code, buffer.str());
  }

  template <class T, int V = 0, typename... Arguments>
  static std::string make_str(Arguments... params) {
    NetMsg msg = NetMsg::make<T, V>(params...);
    std::stringstream buffer;
    msgpack::pack(buffer, msg);
    buffer.seekg(0);
    return buffer.str();
  }

  template <class T, int V = 0, typename... Arguments>
  static NetPayload make_pl(Arguments... params) {
    return NetMsg::make_str<T, V>(params...);
  }

  template <class T, int V = 0, typename... Arguments>
  static QString make_qstr(Arguments... params) {
    return QString::fromStdString(make_str<T, V>(params...));
  }
  
};

namespace payload::CS {

  class Auth {
   public:
    static const quint16 code = CS_AUTH;
    Auth(QString nick = ""): nickname(nick.toStdString()) {}
    std::string nickname;
    MSGPACK_DEFINE_MAP(nickname);
  };

  class MsgPublic {
   public:
    static const quint16 code = CS_MSG_PUBLIC;
    MsgPublic(QString msg = ""): message(msg.toStdString()) {}
    std::string message;
    MSGPACK_DEFINE_MAP(message);
  };

  class MsgPrivate {
   public:
    static const quint16 code = CS_MSG_PRIVATE;
    MsgPrivate(quint16 recv = 0, QString msg = ""):
      receiver(recv), message(msg.toStdString()) {}
    quint16 receiver;
    std::string message;
    MSGPACK_DEFINE_MAP(receiver, message);
  };

}

typedef payload::CS::Auth PlCsAuth;
typedef payload::CS::MsgPublic PlCsMsgPublic;
typedef payload::CS::MsgPrivate PlCsMsgPrivate;

#endif // PACKETS_H

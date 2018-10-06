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

#include <pwmsgpack.h>

msgpack::object_handle msgpack::unpack(QDataStream* s, std::size_t len) {
  std::string str(len, '\0');
  s->readRawData(&str[0], len);
  msgpack::object_handle oh = unpack(str.data(), str.size());
  return oh;
}

msgpack::object_handle msgpack::unpack(QDataStream& s, std::size_t len) {
  return msgpack::unpack(&s, len);
}

msgpack::object_handle msgpack::unpack(QByteArray* b, std::size_t len) {
  msgpack::object_handle oh = unpack(b->data(), len);
  return oh;
}

msgpack::object_handle msgpack::unpack(QByteArray& b, std::size_t len) {
  return msgpack::unpack(&b, len);
}

msgpack::object_handle msgpack::unpack(QChar* s, std::size_t len) {
  return msgpack::unpack((char*) s, len);
}

static bool is_equal(const QString &qs, const std::string &stds) {
  return stds == qs.toStdString();
}

bool operator == (const QString &qs, const std::string &stds) {
  return is_equal(qs, stds);
}

bool operator == (const std::string &stds, const QString &qs) {
  return is_equal(qs, stds);
}

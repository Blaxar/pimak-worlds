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

#include <OgreMath.h>

#include "OgreFrameListener.h"
#include "AnimationManager.h"

OgreFrameListener::OgreFrameListener() {
  ogreControls = new bool[9];
  for (int i=0;i<=9;i++) { ogreControls[i] = false; }
}

OgreFrameListener::~OgreFrameListener() {
  delete [] ogreControls;
}

bool OgreFrameListener::frameStarted(const Ogre::FrameEvent &evt) {
  AnimationManager::getSingleton()->animate(evt);

  return true;
}
bool OgreFrameListener::frameEnded(const Ogre::FrameEvent &evt) {
  Q_UNUSED(&evt);
  return true;
}
bool OgreFrameListener::frameRenderingQueued(const Ogre::FrameEvent &evt) {
  Q_UNUSED(&evt);
  return true;
}

void OgreFrameListener::handleKeys(int key, bool state) {
  switch(key) {
  case Qt::Key_Up:
    ogreControls[UP] = state;
    break;
  case Qt::Key_Right:
    ogreControls[RIGHT] = state;
    break;
  case Qt::Key_Down:
    ogreControls[DOWN] = state;
    break;
  case Qt::Key_Left:
    ogreControls[LEFT] = state;
    break;
  case Qt::Key_PageUp:
    ogreControls[PGUP] = state;
    break;
  case Qt::Key_PageDown:
    ogreControls[PGDOWN] = state;
    break;
  case Qt::Key_Plus:
    ogreControls[PLUS] = state;
    break;
  case Qt::Key_Minus:
    ogreControls[MINUS] = state;
    break;
  case Qt::Key_Control:
    ogreControls[CTRL] = state;
    break;
  case Qt::Key_Shift:
    ogreControls[SHIFT] = state;
    break;
  default:
    break;
  }
}
/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "platform/OSXGestureHelper.h"

namespace deskflow {

OSXPrivateGestureFields readPrivateGestureFields(CGEventRef event)
{
  OSXPrivateGestureFields fields;
  if (event == nullptr) {
    return fields;
  }

  fields.hidType = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::HIDType));
  fields.swipeType = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::SwipeType));
  fields.progress = CGEventGetDoubleValueField(event, static_cast<CGEventField>(GestureFieldID::Progress));
  fields.velocityX = CGEventGetDoubleValueField(event, static_cast<CGEventField>(GestureFieldID::VelocityX));
  fields.velocityY = CGEventGetDoubleValueField(event, static_cast<CGEventField>(GestureFieldID::VelocityY));
  fields.phase = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::Phase));
  fields.motionAxis = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::MotionAxis));

  return fields;
}

bool isGesturePhaseBegan(int64_t phase)
{
  return (phase == 1) || ((phase & 1) != 0);
}

bool isGesturePhaseChanged(int64_t phase)
{
  return (phase == 2);
}

bool isGesturePhaseEnded(int64_t phase)
{
  if (phase == 0) {
    return false;
  }
  return (phase == 4) || ((phase & 4) != 0);
}

bool isGesturePhaseCancelled(int64_t phase)
{
  if (phase == 0) {
    return false;
  }
  return (phase == 8) || (phase == 16) || ((phase & 8) != 0) || ((phase & 16) != 0);
}

bool isThreeFingerDragMetadata(uint32_t type, const OSXPrivateGestureFields &fields)
{
  if (type != 29) {
    return false;
  }
  if (fields.swipeType != static_cast<int64_t>(GestureFieldID::DockSwipeType)) {
    if (fields.hidType == 27 || fields.hidType == 11) {
      return true;
    }
  }
  return false;
}

OSXSwipeDirection classifySwipeDirection(int64_t motionAxis, double progress, double velocityX, double velocityY)
{
  // Fall back safely if motion axis is not set: infer from velocity vector
  if (motionAxis == 0) {
    if (std::abs(velocityX) > std::abs(velocityY) && std::abs(velocityX) > 0.001) {
      motionAxis = 1;
    } else if (std::abs(velocityY) > std::abs(velocityX) && std::abs(velocityY) > 0.001) {
      motionAxis = 2;
    }
  }

  if (motionAxis == 1) { // Horizontal swipe
    double sign = (std::abs(velocityX) > 0.001) ? velocityX : progress;
    if (sign < -0.001) {
      return OSXSwipeDirection::Left;
    }
    if (sign > 0.001) {
      return OSXSwipeDirection::Right;
    }
  } else if (motionAxis == 2) { // Vertical swipe
    double sign = (std::abs(velocityY) > 0.001) ? velocityY : progress;
    if (sign > 0.001) {
      return OSXSwipeDirection::Up;
    }
    if (sign < -0.001) {
      return OSXSwipeDirection::Down;
    }
  }

  return OSXSwipeDirection::None;
}

bool getKeystrokeForSwipe(OSXSwipeDirection direction, KeyID &outKey, KeyModifierMask &outMask)
{
  outMask = KeyModifierControl;
  switch (direction) {
  case OSXSwipeDirection::Up:
    outKey = kKeyUp;
    return true;
  case OSXSwipeDirection::Down:
    outKey = kKeyDown;
    return true;
  case OSXSwipeDirection::Left:
    // Natural direction: swiping left moves to the Space on the right
    outKey = kKeyRight;
    return true;
  case OSXSwipeDirection::Right:
    // Natural direction: swiping right moves to the Space on the left
    outKey = kKeyLeft;
    return true;
  case OSXSwipeDirection::None:
  default:
    outKey = kKeyNone;
    outMask = 0;
    return false;
  }
}

const char *swipeDirectionToString(OSXSwipeDirection direction)
{
  switch (direction) {
  case OSXSwipeDirection::Up:
    return "Up (Mission Control)";
  case OSXSwipeDirection::Down:
    return "Down (App Expose)";
  case OSXSwipeDirection::Left:
    return "Left (Space Right)";
  case OSXSwipeDirection::Right:
    return "Right (Space Left)";
  case OSXSwipeDirection::None:
  default:
    return "None";
  }
}

} // namespace deskflow

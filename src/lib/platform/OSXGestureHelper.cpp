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
  fields.swipeMotionAxis =
      CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::SwipeMotionAxis));
  fields.progress = CGEventGetDoubleValueField(event, static_cast<CGEventField>(GestureFieldID::Progress));
  fields.velocity = CGEventGetDoubleValueField(event, static_cast<CGEventField>(GestureFieldID::Velocity));
  fields.phase = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::Phase));
  fields.motionAxis = CGEventGetIntegerValueField(event, static_cast<CGEventField>(GestureFieldID::MotionAxis));

  // Fall back to field 123 when field 165 is 0
  if (fields.motionAxis == 0) {
    fields.motionAxis = fields.swipeMotionAxis;
  }

  return fields;
}

bool isGesturePhaseCancelled(int64_t phase)
{
  if (phase <= 0) {
    return false;
  }
  return (phase & (GesturePhase::Cancelled | GesturePhase::CancelledNSEvent)) != 0;
}

bool isGesturePhaseEnded(int64_t phase)
{
  if (phase <= 0 || isGesturePhaseCancelled(phase)) {
    return false;
  }
  return (phase & GesturePhase::Ended) != 0;
}

bool isGesturePhaseChanged(int64_t phase)
{
  if (phase <= 0 || isGesturePhaseCancelled(phase) || isGesturePhaseEnded(phase)) {
    return false;
  }
  return (phase & GesturePhase::Changed) != 0;
}

bool isGesturePhaseBegan(int64_t phase)
{
  if (phase <= 0 || isGesturePhaseCancelled(phase) || isGesturePhaseEnded(phase) || isGesturePhaseChanged(phase)) {
    return false;
  }
  return (phase & GesturePhase::Began) != 0;
}

bool isThreeFingerDragGesture(uint32_t type, const OSXPrivateGestureFields &fields, int touchCount)
{
  if (type != 29) {
    return false;
  }
  // If touch count is known, it must be exactly 3.
  if (touchCount > 0 && touchCount != 3) {
    return false;
  }
  // Must NOT be a dock swipe, navigation swipe, or axis-bound gesture.
  if (fields.swipeMotionAxis != 0 || fields.motionAxis != 0 || fields.hidType == 23) {
    return false;
  }
  // Must have fluid touch HID type (27) or digitizer (11).
  if (fields.hidType != 27 && fields.hidType != 11) {
    return false;
  }
  // Must be in began or changed phase.
  if (!isGesturePhaseBegan(fields.phase) && !isGesturePhaseChanged(fields.phase)) {
    return false;
  }
  return true;
}

OSXSwipeDirection classifySwipeDirection(int64_t motionAxis, double progress, double velocity)
{
  if (motionAxis == 1) { // Horizontal swipe
    double sign = (std::abs(progress) > 0.001) ? progress : velocity;
    if (sign < -0.001) {
      return OSXSwipeDirection::Left;
    }
    if (sign > 0.001) {
      return OSXSwipeDirection::Right;
    }
  } else if (motionAxis == 2) { // Vertical swipe
    // Negative progress = swipe UP (Mission Control); positive progress = swipe DOWN (App Exposé)
    double sign = (std::abs(progress) > 0.001) ? progress : velocity;
    if (sign < -0.001) {
      return OSXSwipeDirection::Up;
    }
    if (sign > 0.001) {
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

void OSXDockSwipeTracker::reset()
{
  m_active = false;
  m_sawChanged = false;
  m_axis = 0;
  m_progress = 0.0;
  m_velocity = 0.0;
}

bool OSXDockSwipeTracker::processEvent(const OSXPrivateGestureFields &fields, OSXSwipeDirection &outDirection)
{
  outDirection = OSXSwipeDirection::None;

  if (isGesturePhaseCancelled(fields.phase)) {
    if (m_active) {
      reset();
    }
    return false;
  }

  int64_t axis = (fields.motionAxis != 0) ? fields.motionAxis : fields.swipeMotionAxis;

  if (isGesturePhaseBegan(fields.phase)) {
    m_active = true;
    m_sawChanged = false;
    m_axis = axis;
    m_progress = fields.progress;
    m_velocity = fields.velocity;
    return false;
  }

  if (isGesturePhaseChanged(fields.phase)) {
    m_active = true;
    m_sawChanged = true;
    if (axis != 0) {
      m_axis = axis;
    }
    if (std::abs(fields.progress) > 0.001) {
      m_progress = fields.progress;
    }
    if (std::abs(fields.velocity) > 0.001) {
      m_velocity = fields.velocity;
    }
    return false;
  }

  if (isGesturePhaseEnded(fields.phase)) {
    if (!m_active) {
      // Duplicate ended or ended without began: ignore
      return false;
    }

    if (axis != 0) {
      m_axis = axis;
    }
    if (std::abs(fields.progress) > 0.001) {
      m_progress = fields.progress;
    }
    if (std::abs(fields.velocity) > 0.001) {
      m_velocity = fields.velocity;
    }

    bool wasChanged = m_sawChanged;
    double finalProgress = m_progress;
    double finalVelocity = m_velocity;
    int64_t finalAxis = m_axis;

    reset();

    // If began -> ended directly without changed, require that the ended event
    // has a non-zero progress or velocity to establish motion.
    if (!wasChanged && std::abs(finalProgress) <= 0.001 && std::abs(finalVelocity) <= 0.001) {
      return false;
    }

    OSXSwipeDirection dir = classifySwipeDirection(finalAxis, finalProgress, finalVelocity);
    if (dir != OSXSwipeDirection::None) {
      outDirection = dir;
      return true;
    }
    return false;
  }

  return false;
}

} // namespace deskflow

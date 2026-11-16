/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include "deskflow/KeyTypes.h"

#include <ApplicationServices/ApplicationServices.h>
#include <cmath>
#include <cstdint>
#include <mutex>

namespace deskflow {

// Private CoreGraphics / SkyLight event field IDs for trackpad gestures.
// Undocumented SPI fields used by macOS WindowServer and Dock:
//   110: kCGEventGestureHIDType (HID type: 23 = dock swipe, 27 = fluid touch)
//   123: kCGEventGestureSwipeMotionAxis (swipe motion axis: 1 = horizontal, 2 = vertical)
//   124: kCGEventGestureSwipeProgress (scalar progress along motion axis)
//   125: kCGEventGestureSwipePositionX
//   126: kCGEventGestureSwipePositionY
//   129: kCGEventGestureSwipeVelocity (scalar velocity along motion axis)
//   132: kCGEventGesturePhase (CGSGesturePhase bitmask/values: 1=began, 2=changed, 4=ended, 8=cancelled)
//   165: kCGEventGestureMotionAxis (motion axis: 1=horizontal, 2=vertical)
//   170: kCGEventUnacceleratedPointerMovementX (raw horizontal delta)
//   171: kCGEventUnacceleratedPointerMovementY (raw vertical delta)
namespace GestureFieldID {
constexpr uint32_t HIDType = 110;
constexpr uint32_t SwipeMotionAxis = 123;
constexpr uint32_t Progress = 124;
constexpr uint32_t PositionX = 125;
constexpr uint32_t PositionY = 126;
constexpr uint32_t Velocity = 129;
constexpr uint32_t Phase = 132;
constexpr uint32_t MotionAxis = 165;
constexpr uint32_t UnacceleratedX = 170;
constexpr uint32_t UnacceleratedY = 171;
constexpr int64_t DockSwipeHIDType = 23;
} // namespace GestureFieldID

namespace GesturePhase {
constexpr int64_t None = 0;
constexpr int64_t Began = 1;             // 1 << 0
constexpr int64_t Changed = 2;           // 1 << 1
constexpr int64_t Ended = 4;             // 1 << 2
constexpr int64_t Cancelled = 8;         // 1 << 3 (CGSGesturePhaseCancelled)
constexpr int64_t CancelledNSEvent = 16; // 1 << 4 (NSEventPhaseCancelled)
} // namespace GesturePhase

struct OSXPrivateGestureFields
{
  int64_t hidType = 0;
  int64_t swipeMotionAxis = 0;
  double progress = 0.0;
  double velocity = 0.0;
  int64_t phase = 0;
  int64_t motionAxis = 0;
};

enum class OSXSwipeDirection
{
  None,
  Up,
  Down,
  Left,
  Right
};

// Reads private CoreGraphics gesture fields from CGEventRef through one small helper.
// Falls back safely to zero values if event is null or fields read 0.
// Axis resolution: uses field 165, falling back to field 123 when 165 is 0.
OSXPrivateGestureFields readPrivateGestureFields(CGEventRef event);

// Pure helpers for gesture phase classification.
// Hierarchy: Cancelled > Ended > Changed > Began.
bool isGesturePhaseBegan(int64_t phase);
bool isGesturePhaseChanged(int64_t phase);
bool isGesturePhaseEnded(int64_t phase);
bool isGesturePhaseCancelled(int64_t phase);

// Classifies swipe direction based strictly on motion axis (1=horizontal, 2=vertical)
// and sign of progress (scalar along motion axis), using velocity as a scalar tie-breaker.
// Horizontal (axis 1): progress < 0 -> Left, progress > 0 -> Right
// Vertical (axis 2): progress < 0 -> Up (Mission Control), progress > 0 -> Down (App Exposé)
// Returns None if axis is unknown or direction cannot be determined.
OSXSwipeDirection classifySwipeDirection(int64_t motionAxis, double progress, double velocity);

// Determines if event represents a three-finger drag gesture.
// Only returns true for began or changed phase with fluid touch HID type (27 or 11),
// zero swipe motion axis, zero motion axis, hidType != 23, and touch count of 3 (or unspecified -1).
bool isThreeFingerDragGesture(uint32_t type, const OSXPrivateGestureFields &fields, int touchCount = -1);

// Maps a swipe direction to the keystroke that triggers the corresponding client action.
// Up -> Ctrl+Up (Mission Control)
// Down -> Ctrl+Down (App Exposé)
// Left -> Ctrl+Right (Spaces: swipe left moves to Space on the right)
// Right -> Ctrl+Left (Spaces: swipe right moves to Space on the left)
bool getKeystrokeForSwipe(OSXSwipeDirection direction, KeyID &outKey, KeyModifierMask &outMask);

// Returns a human-readable name for the swipe direction.
const char *swipeDirectionToString(OSXSwipeDirection direction);

// State machine tracker for macOS dock swipes.
// Tracks began -> changed -> ended lifecycle.
// Rejects incomplete gestures (e.g. began -> ended without motion) and duplicate ended events.
class OSXDockSwipeTracker
{
public:
  // Processes an event's gesture fields.
  // Returns true and sets outDirection if a valid completed swipe gesture ended.
  bool processEvent(const OSXPrivateGestureFields &fields, OSXSwipeDirection &outDirection);

  void reset();

  bool isActive() const
  {
    return m_active;
  }
  bool sawChanged() const
  {
    return m_sawChanged;
  }

private:
  bool m_active = false;
  bool m_sawChanged = false;
  int64_t m_axis = 0;
  double m_progress = 0.0;
  double m_velocity = 0.0;
};

// Thread-safe tracker for three-finger drag state and position tracking.
// Serializes flag transitions, position tracking, and button event synchronization.
class OSXThreeFingerDragTracker
{
public:
  OSXThreeFingerDragTracker() = default;

  // Thread-safe external API
  bool startDrag()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return startDragUnlocked();
  }

  bool cancelDrag()
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return cancelDragUnlocked();
  }

  bool isDragging() const
  {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_dragging;
  }

  // Unlocked primitives for compound atomic operations with mutex()
  bool startDragUnlocked()
  {
    if (!m_dragging) {
      m_dragging = true;
      m_hasLastPos = false;
      return true;
    }
    return false;
  }

  bool cancelDragUnlocked()
  {
    if (m_dragging) {
      m_dragging = false;
      m_hasLastPos = false;
      return true;
    }
    return false;
  }

  bool isDraggingUnlocked() const
  {
    return m_dragging;
  }

  bool hasLastPositionUnlocked() const
  {
    return m_hasLastPos;
  }

  CGPoint lastPositionUnlocked() const
  {
    return m_lastPos;
  }

  void setLastPositionUnlocked(CGPoint pos)
  {
    m_lastPos = pos;
    m_hasLastPos = true;
  }

  void resetPositionUnlocked()
  {
    m_hasLastPos = false;
  }

  std::mutex &mutex()
  {
    return m_mutex;
  }

private:
  mutable std::mutex m_mutex;
  bool m_dragging = false;
  CGPoint m_lastPos = CGPointZero;
  bool m_hasLastPos = false;
};

} // namespace deskflow

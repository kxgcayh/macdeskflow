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

namespace deskflow {

// Private CoreGraphics / SkyLight event field IDs for trackpad gestures.
// Undocumented SPI fields used by macOS WindowServer and Dock:
//   110: kCGEventGestureHIDType (e.g. 23 = kIOHIDEventTypeDockSwipe)
//   123: kCGEventGestureSwipeMotion / swipe type (23 = dock swipe)
//   124: kCGEventGestureSwipeProgress (scalar progress value)
//   125: kCGEventGestureSwipePositionX
//   126: kCGEventGestureSwipePositionY
//   129: kCGEventGestureSwipeVelocityX (horizontal velocity)
//   130: kCGEventGestureSwipeVelocityY (vertical velocity)
//   132: kCGEventGesturePhase (CGSGesturePhase: 1=began, 2=changed, 4=ended, 8=cancelled)
//   165: kCGEventGestureMotionAxis (1=horizontal, 2=vertical)
//   170: kCGEventUnacceleratedPointerMovementX (raw horizontal delta)
//   171: kCGEventUnacceleratedPointerMovementY (raw vertical delta)
namespace GestureFieldID {
constexpr uint32_t HIDType = 110;
constexpr uint32_t SwipeType = 123;
constexpr uint32_t Progress = 124;
constexpr uint32_t PositionX = 125;
constexpr uint32_t PositionY = 126;
constexpr uint32_t VelocityX = 129;
constexpr uint32_t VelocityY = 130;
constexpr uint32_t Phase = 132;
constexpr uint32_t MotionAxis = 165;
constexpr uint32_t UnacceleratedX = 170;
constexpr uint32_t UnacceleratedY = 171;
constexpr int64_t DockSwipeType = 23;
} // namespace GestureFieldID

struct OSXPrivateGestureFields
{
  int64_t hidType = 0;
  int64_t swipeType = 0;
  double progress = 0.0;
  double velocityX = 0.0;
  double velocityY = 0.0;
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
OSXPrivateGestureFields readPrivateGestureFields(CGEventRef event);

// Pure helper: checks if phase indicates gesture began (phase 1)
bool isGesturePhaseBegan(int64_t phase);

// Pure helper: checks if phase indicates gesture changed / active (phase 2)
bool isGesturePhaseChanged(int64_t phase);

// Pure helper: checks if phase indicates gesture ended successfully (phase 4 in CGSGesturePhase)
bool isGesturePhaseEnded(int64_t phase, bool sawPhaseChanged = true);

// Pure helper: checks if phase indicates gesture was cancelled (phase 8 in CGS, 16 in NSEvent)
bool isGesturePhaseCancelled(int64_t phase, bool sawPhaseChanged = true);

// Pure helper: classifies swipe direction based on motion axis, progress, and velocity components.
// Returns None if axis or direction cannot be determined.
OSXSwipeDirection classifySwipeDirection(int64_t motionAxis, double progress, double velocityX, double velocityY);

// Maps a swipe direction to the keystroke that triggers the corresponding client action.
// Up -> Ctrl+Up (Mission Control)
// Down -> Ctrl+Down (App Exposé)
// Left -> Ctrl+Right (Spaces: swipe left moves to Space on the right)
// Right -> Ctrl+Left (Spaces: swipe right moves to Space on the left)
bool getKeystrokeForSwipe(OSXSwipeDirection direction, KeyID &outKey, KeyModifierMask &outMask);

// Returns a human-readable name for the swipe direction.
const char *swipeDirectionToString(OSXSwipeDirection direction);

} // namespace deskflow

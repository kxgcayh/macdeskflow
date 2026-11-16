/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXGestureHelperTests.h"

#include "platform/OSXGestureHelper.h"

using namespace deskflow;

void OSXGestureHelperTests::classifyHorizontalSwipes()
{
  // Horizontal axis is 1
  // Negative velocity/progress indicates swipe left
  QCOMPARE(classifySwipeDirection(1, 0.0, -500.0, 0.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(1, -1.0, 0.0, 0.0), OSXSwipeDirection::Left);

  // Positive velocity/progress indicates swipe right
  QCOMPARE(classifySwipeDirection(1, 0.0, 500.0, 0.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(1, 1.0, 0.0, 0.0), OSXSwipeDirection::Right);

  // Zero progress and zero velocity returns None
  QCOMPARE(classifySwipeDirection(1, 0.0, 0.0, 0.0), OSXSwipeDirection::None);
}

void OSXGestureHelperTests::classifyVerticalSwipes()
{
  // Vertical axis is 2
  // Positive velocity/progress indicates swipe up (Mission Control)
  QCOMPARE(classifySwipeDirection(2, 0.0, 0.0, 500.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, 1.0, 0.0, 0.0), OSXSwipeDirection::Up);

  // Negative velocity/progress indicates swipe down (App Exposé)
  QCOMPARE(classifySwipeDirection(2, 0.0, 0.0, -500.0), OSXSwipeDirection::Down);
  QCOMPARE(classifySwipeDirection(2, -1.0, 0.0, 0.0), OSXSwipeDirection::Down);

  // Zero progress and zero velocity returns None
  QCOMPARE(classifySwipeDirection(2, 0.0, 0.0, 0.0), OSXSwipeDirection::None);
}

void OSXGestureHelperTests::classifyFallbackZeroAxis()
{
  // When axis is 0, infer from velocity vector
  QCOMPARE(classifySwipeDirection(0, 0.0, -600.0, 10.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(0, 0.0, 600.0, 10.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(0, 0.0, 10.0, 600.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(0, 0.0, 10.0, -600.0), OSXSwipeDirection::Down);

  // If all vectors are zero, return None safely
  QCOMPARE(classifySwipeDirection(0, 0.0, 0.0, 0.0), OSXSwipeDirection::None);
}

void OSXGestureHelperTests::keystrokeMapping()
{
  KeyID key = kKeyNone;
  KeyModifierMask mask = 0;

  // Up -> Ctrl+Up (Mission Control)
  QVERIFY(getKeystrokeForSwipe(OSXSwipeDirection::Up, key, mask));
  QCOMPARE(key, kKeyUp);
  QCOMPARE(mask, KeyModifierControl);

  // Down -> Ctrl+Down (App Exposé)
  QVERIFY(getKeystrokeForSwipe(OSXSwipeDirection::Down, key, mask));
  QCOMPARE(key, kKeyDown);
  QCOMPARE(mask, KeyModifierControl);

  // Left swipe -> Space to the right (Ctrl+Right)
  QVERIFY(getKeystrokeForSwipe(OSXSwipeDirection::Left, key, mask));
  QCOMPARE(key, kKeyRight);
  QCOMPARE(mask, KeyModifierControl);

  // Right swipe -> Space to the left (Ctrl+Left)
  QVERIFY(getKeystrokeForSwipe(OSXSwipeDirection::Right, key, mask));
  QCOMPARE(key, kKeyLeft);
  QCOMPARE(mask, KeyModifierControl);

  // None -> returns false
  QVERIFY(!getKeystrokeForSwipe(OSXSwipeDirection::None, key, mask));
}

void OSXGestureHelperTests::gesturePhases()
{
  // Began: phase 1
  QVERIFY(isGesturePhaseBegan(1));
  QVERIFY(!isGesturePhaseBegan(0));
  QVERIFY(!isGesturePhaseBegan(2));
  QVERIFY(!isGesturePhaseBegan(4));

  // Changed: phase 2
  QVERIFY(isGesturePhaseChanged(2));
  QVERIFY(!isGesturePhaseChanged(0));
  QVERIFY(!isGesturePhaseChanged(1));
  QVERIFY(!isGesturePhaseChanged(4));

  // Ended: phase 4 (phase 8 must NEVER be treated as ended)
  QVERIFY(isGesturePhaseEnded(4));
  QVERIFY(!isGesturePhaseEnded(0));
  QVERIFY(!isGesturePhaseEnded(2));
  QVERIFY(!isGesturePhaseEnded(8));

  // Cancelled: phase 8 or 16
  QVERIFY(isGesturePhaseCancelled(8));
  QVERIFY(isGesturePhaseCancelled(16));
  QVERIFY(!isGesturePhaseCancelled(0));
  QVERIFY(!isGesturePhaseCancelled(4));
}

void OSXGestureHelperTests::threeFingerDragMetadata()
{
  OSXPrivateGestureFields fields;
  fields.hidType = 27; // kIOHIDEventTypeFluidTouchGesture
  fields.swipeType = 0;

  // Type 29 with fluid touch is recognized
  QVERIFY(isThreeFingerDragMetadata(29, fields));

  // Digitizer type 11
  fields.hidType = 11;
  QVERIFY(isThreeFingerDragMetadata(29, fields));

  // Dock swipe (swipe type 23) must not be recognized as drag
  fields.swipeType = 23;
  QVERIFY(!isThreeFingerDragMetadata(29, fields));

  // Non-gesture event types must not be recognized as drag
  fields.swipeType = 0;
  QVERIFY(!isThreeFingerDragMetadata(30, fields));
  QVERIFY(!isThreeFingerDragMetadata(1, fields));
}

void OSXGestureHelperTests::nullEventSafe()
{
  OSXPrivateGestureFields fields = readPrivateGestureFields(nullptr);
  QCOMPARE(fields.hidType, 0LL);
  QCOMPARE(fields.swipeType, 0LL);
  QCOMPARE(fields.progress, 0.0);
  QCOMPARE(fields.velocityX, 0.0);
  QCOMPARE(fields.velocityY, 0.0);
  QCOMPARE(fields.phase, 0LL);
  QCOMPARE(fields.motionAxis, 0LL);
}

QTEST_MAIN(OSXGestureHelperTests)

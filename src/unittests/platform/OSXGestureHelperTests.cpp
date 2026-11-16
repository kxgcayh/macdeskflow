/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXGestureHelperTests.h"

#include "platform/OSXGestureHelper.h"

using namespace deskflow;

void OSXGestureHelperTests::classifyAllFourSwipeDirections()
{
  // Horizontal axis is 1: progress determines direction
  // Real hardware value: horizontal negative progress (-0.25) = swipe RIGHT
  QCOMPARE(classifySwipeDirection(1, -0.25, -1.20), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(1, -1.0, 0.0), OSXSwipeDirection::Right);

  // Horizontal positive progress = swipe LEFT
  QCOMPARE(classifySwipeDirection(1, 0.50, 2.10), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(1, 1.0, 0.0), OSXSwipeDirection::Left);

  // Vertical axis is 2: progress determines direction
  // Real hardware value: vertical with negative progress (-0.54) = swipe UP (Mission Control)
  QCOMPARE(classifySwipeDirection(2, -0.54, -2.35), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, -0.028, 0.0), OSXSwipeDirection::Up);

  // Real hardware value: vertical with positive progress (+0.72) = swipe DOWN (App Exposé)
  QCOMPARE(classifySwipeDirection(2, 0.72, 3.10), OSXSwipeDirection::Down);
  QCOMPARE(classifySwipeDirection(2, 1.0, 0.0), OSXSwipeDirection::Down);
}

void OSXGestureHelperTests::classifyVelocityTieBreaker()
{
  // When progress is approximately zero (<= 0.001), velocity acts as scalar tie-breaker
  // Horizontal axis (1): negative velocity = Right, positive velocity = Left
  QCOMPARE(classifySwipeDirection(1, 0.0, -500.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(1, 0.0, 500.0), OSXSwipeDirection::Left);

  // Vertical axis (2): negative velocity = Up, positive velocity = Down
  QCOMPARE(classifySwipeDirection(2, 0.0, -500.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, 0.0, 500.0), OSXSwipeDirection::Down);

  // When progress is non-zero, progress takes precedence over velocity
  QCOMPARE(classifySwipeDirection(1, -0.25, 500.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(1, 0.50, -500.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(2, -0.54, 500.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, 0.72, -500.0), OSXSwipeDirection::Down);
}

void OSXGestureHelperTests::classifyInvalidAxisAndZeroMovement()
{
  // Unknown or invalid motion axes must return None
  QCOMPARE(classifySwipeDirection(0, 1.0, 100.0), OSXSwipeDirection::None);
  QCOMPARE(classifySwipeDirection(3, 1.0, 100.0), OSXSwipeDirection::None);
  QCOMPARE(classifySwipeDirection(-1, 1.0, 100.0), OSXSwipeDirection::None);

  // Both progress and velocity at zero return None
  QCOMPARE(classifySwipeDirection(1, 0.0, 0.0), OSXSwipeDirection::None);
  QCOMPARE(classifySwipeDirection(2, 0.0, 0.0), OSXSwipeDirection::None);
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

void OSXGestureHelperTests::gesturePhasesHierarchy()
{
  // Exact single phases
  // Began: phase 1
  QVERIFY(isGesturePhaseBegan(GesturePhase::Began));
  QVERIFY(!isGesturePhaseChanged(GesturePhase::Began));
  QVERIFY(!isGesturePhaseEnded(GesturePhase::Began));
  QVERIFY(!isGesturePhaseCancelled(GesturePhase::Began));

  // Changed: phase 2
  QVERIFY(isGesturePhaseChanged(GesturePhase::Changed));
  QVERIFY(!isGesturePhaseBegan(GesturePhase::Changed));
  QVERIFY(!isGesturePhaseEnded(GesturePhase::Changed));
  QVERIFY(!isGesturePhaseCancelled(GesturePhase::Changed));

  // Ended: phase 4
  QVERIFY(isGesturePhaseEnded(GesturePhase::Ended));
  QVERIFY(!isGesturePhaseBegan(GesturePhase::Ended));
  QVERIFY(!isGesturePhaseChanged(GesturePhase::Ended));
  QVERIFY(!isGesturePhaseCancelled(GesturePhase::Ended));

  // Cancelled: phase 8 (CGS) and 16 (NSEvent)
  QVERIFY(isGesturePhaseCancelled(GesturePhase::Cancelled));
  QVERIFY(isGesturePhaseCancelled(GesturePhase::CancelledNSEvent));
  QVERIFY(!isGesturePhaseBegan(GesturePhase::Cancelled));
  QVERIFY(!isGesturePhaseChanged(GesturePhase::Cancelled));
  QVERIFY(!isGesturePhaseEnded(GesturePhase::Cancelled));

  // Bitmask combinations: Cancelled > Ended > Changed > Began
  // Began | Ended (1 | 4 = 5) -> Ended takes precedence
  QVERIFY(isGesturePhaseEnded(5));
  QVERIFY(!isGesturePhaseBegan(5));
  QVERIFY(!isGesturePhaseChanged(5));
  QVERIFY(!isGesturePhaseCancelled(5));

  // Changed | Ended (2 | 4 = 6) -> Ended takes precedence
  QVERIFY(isGesturePhaseEnded(6));
  QVERIFY(!isGesturePhaseChanged(6));

  // Ended | Cancelled (4 | 8 = 12) -> Cancelled takes precedence
  QVERIFY(isGesturePhaseCancelled(12));
  QVERIFY(!isGesturePhaseEnded(12));

  // Began | Cancelled (1 | 8 = 9) -> Cancelled takes precedence
  QVERIFY(isGesturePhaseCancelled(9));
  QVERIFY(!isGesturePhaseBegan(9));

  // Unknown / unhandled flags
  QVERIFY(!isGesturePhaseBegan(0));
  QVERIFY(!isGesturePhaseChanged(0));
  QVERIFY(!isGesturePhaseEnded(0));
  QVERIFY(!isGesturePhaseCancelled(0));

  QVERIFY(!isGesturePhaseBegan(32));
  QVERIFY(!isGesturePhaseChanged(32));
  QVERIFY(!isGesturePhaseEnded(32));
  QVERIFY(!isGesturePhaseCancelled(32));
}

void OSXGestureHelperTests::dockSwipeStateMachine()
{
  OSXDockSwipeTracker tracker;
  OSXSwipeDirection dir = OSXSwipeDirection::None;

  // 1. Began -> Ended without changed and without movement: sends nothing
  OSXPrivateGestureFields began;
  began.phase = GesturePhase::Began;
  began.motionAxis = 2;
  began.progress = 0.0;
  began.velocity = 0.0;
  QVERIFY(!tracker.processEvent(began, dir));
  QVERIFY(tracker.isActive());
  QVERIFY(!tracker.sawChanged());

  OSXPrivateGestureFields endedZero;
  endedZero.phase = GesturePhase::Ended;
  endedZero.motionAxis = 2;
  endedZero.progress = 0.0;
  endedZero.velocity = 0.0;
  QVERIFY(!tracker.processEvent(endedZero, dir));
  QCOMPARE(dir, OSXSwipeDirection::None);
  QVERIFY(!tracker.isActive());

  // 2. Real vertical sequence: begin -0.028, end -0.54 vertical = Up
  OSXPrivateGestureFields beganUp;
  beganUp.phase = GesturePhase::Began;
  beganUp.motionAxis = 2;
  beganUp.progress = -0.028473;
  beganUp.velocity = 0.0;
  QVERIFY(!tracker.processEvent(beganUp, dir));
  QVERIFY(tracker.isActive());

  OSXPrivateGestureFields changedUp;
  changedUp.phase = GesturePhase::Changed;
  changedUp.motionAxis = 2;
  changedUp.progress = -0.362259;
  changedUp.velocity = 0.0;
  QVERIFY(!tracker.processEvent(changedUp, dir));
  QVERIFY(tracker.sawChanged());

  OSXPrivateGestureFields endedUp;
  endedUp.phase = GesturePhase::Ended;
  endedUp.motionAxis = 2;
  endedUp.progress = -0.540726;
  endedUp.velocity = -2.348709;
  QVERIFY(tracker.processEvent(endedUp, dir));
  QCOMPARE(dir, OSXSwipeDirection::Up);
  QVERIFY(!tracker.isActive());

  // 3. Real vertical sequence: end +0.72 vertical = Down
  OSXPrivateGestureFields beganDown;
  beganDown.phase = GesturePhase::Began;
  beganDown.motionAxis = 2;
  beganDown.progress = 0.03;
  QVERIFY(!tracker.processEvent(beganDown, dir));

  OSXPrivateGestureFields changedDown;
  changedDown.phase = GesturePhase::Changed;
  changedDown.motionAxis = 2;
  changedDown.progress = 0.45;
  QVERIFY(!tracker.processEvent(changedDown, dir));

  OSXPrivateGestureFields endedDown;
  endedDown.phase = GesturePhase::Ended;
  endedDown.motionAxis = 2;
  endedDown.progress = 0.72;
  endedDown.velocity = 3.10;
  QVERIFY(tracker.processEvent(endedDown, dir));
  QCOMPARE(dir, OSXSwipeDirection::Down);
  QVERIFY(!tracker.isActive());

  // 4. Real horizontal sequence: end -0.25 horizontal = Right
  OSXPrivateGestureFields beganHorizontal;
  beganHorizontal.phase = GesturePhase::Began;
  beganHorizontal.motionAxis = 1;
  beganHorizontal.progress = -0.05;
  QVERIFY(!tracker.processEvent(beganHorizontal, dir));

  OSXPrivateGestureFields changedHorizontal;
  changedHorizontal.phase = GesturePhase::Changed;
  changedHorizontal.motionAxis = 1;
  changedHorizontal.progress = -0.15;
  QVERIFY(!tracker.processEvent(changedHorizontal, dir));

  OSXPrivateGestureFields endedHorizontal;
  endedHorizontal.phase = GesturePhase::Ended;
  endedHorizontal.motionAxis = 1;
  endedHorizontal.progress = -0.25;
  endedHorizontal.velocity = -1.20;
  QVERIFY(tracker.processEvent(endedHorizontal, dir));
  QCOMPARE(dir, OSXSwipeDirection::Right);
  QVERIFY(!tracker.isActive());

  // 5. Cancelled (8) sends nothing
  OSXPrivateGestureFields beganCancel;
  beganCancel.phase = GesturePhase::Began;
  beganCancel.motionAxis = 2;
  beganCancel.progress = -0.028;
  QVERIFY(!tracker.processEvent(beganCancel, dir));
  QVERIFY(tracker.isActive());

  OSXPrivateGestureFields cancelled;
  cancelled.phase = 8;
  QVERIFY(!tracker.processEvent(cancelled, dir));
  QVERIFY(!tracker.isActive());
  QCOMPARE(dir, OSXSwipeDirection::None);

  // 6. Axis fallback: field 165 is 0, field 123 is 2 -> vertical Up
  OSXPrivateGestureFields beganFallback;
  beganFallback.phase = GesturePhase::Began;
  beganFallback.motionAxis = 0;
  beganFallback.swipeMotionAxis = 2;
  beganFallback.progress = -0.028;
  QVERIFY(!tracker.processEvent(beganFallback, dir));

  OSXPrivateGestureFields changedFallback;
  changedFallback.phase = GesturePhase::Changed;
  changedFallback.motionAxis = 0;
  changedFallback.swipeMotionAxis = 2;
  changedFallback.progress = -0.36;
  QVERIFY(!tracker.processEvent(changedFallback, dir));

  OSXPrivateGestureFields endedFallback;
  endedFallback.phase = GesturePhase::Ended;
  endedFallback.motionAxis = 0;
  endedFallback.swipeMotionAxis = 2;
  endedFallback.progress = -0.54;
  QVERIFY(tracker.processEvent(endedFallback, dir));
  QCOMPARE(dir, OSXSwipeDirection::Up);
  QVERIFY(!tracker.isActive());

  // 7. Duplicate Ended: must be ignored
  QVERIFY(!tracker.processEvent(endedUp, dir));

  // 8. Ended without Began: must be ignored
  tracker.reset();
  QVERIFY(!tracker.processEvent(endedUp, dir));
}

void OSXGestureHelperTests::nullEventSafe()
{
  OSXPrivateGestureFields fields = readPrivateGestureFields(nullptr);
  QCOMPARE(fields.hidType, 0LL);
  QCOMPARE(fields.swipeMotionAxis, 0LL);
  QCOMPARE(fields.progress, 0.0);
  QCOMPARE(fields.velocity, 0.0);
  QCOMPARE(fields.phase, 0LL);
  QCOMPARE(fields.motionAxis, 0LL);
}

QTEST_MAIN(OSXGestureHelperTests)

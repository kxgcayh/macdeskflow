/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXGestureHelperTests.h"

#include "platform/OSXGestureHelper.h"

#include <atomic>
#include <thread>
#include <vector>

using namespace deskflow;

void OSXGestureHelperTests::classifyAllFourSwipeDirections()
{
  // Horizontal axis is 1: progress determines direction
  // Progress < 0: Left
  QCOMPARE(classifySwipeDirection(1, -1.0, 0.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(1, -0.05, 0.0), OSXSwipeDirection::Left);

  // Progress > 0: Right
  QCOMPARE(classifySwipeDirection(1, 1.0, 0.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(1, 0.05, 0.0), OSXSwipeDirection::Right);

  // Vertical axis is 2: progress determines direction
  // Progress > 0: Up (Mission Control)
  QCOMPARE(classifySwipeDirection(2, 1.0, 0.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, 0.05, 0.0), OSXSwipeDirection::Up);

  // Progress < 0: Down (App Exposé)
  QCOMPARE(classifySwipeDirection(2, -1.0, 0.0), OSXSwipeDirection::Down);
  QCOMPARE(classifySwipeDirection(2, -0.05, 0.0), OSXSwipeDirection::Down);
}

void OSXGestureHelperTests::classifyVelocityTieBreaker()
{
  // When progress is approximately zero (<= 0.001), velocity acts as scalar tie-breaker
  // Horizontal axis (1)
  QCOMPARE(classifySwipeDirection(1, 0.0, -500.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(1, 0.0, 500.0), OSXSwipeDirection::Right);

  // Vertical axis (2)
  QCOMPARE(classifySwipeDirection(2, 0.0, 500.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, 0.0, -500.0), OSXSwipeDirection::Down);

  // When progress is non-zero, progress takes precedence over velocity
  QCOMPARE(classifySwipeDirection(1, -0.5, 500.0), OSXSwipeDirection::Left);
  QCOMPARE(classifySwipeDirection(1, 0.5, -500.0), OSXSwipeDirection::Right);
  QCOMPARE(classifySwipeDirection(2, 0.5, -500.0), OSXSwipeDirection::Up);
  QCOMPARE(classifySwipeDirection(2, -0.5, 500.0), OSXSwipeDirection::Down);
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

void OSXGestureHelperTests::threeFingerDragGestureFilter()
{
  OSXPrivateGestureFields fields;
  fields.hidType = 27; // Fluid touch
  fields.swipeType = 0;
  fields.motionAxis = 0;
  fields.phase = GesturePhase::Began;

  // Valid 3-finger drag during Began
  QVERIFY(isThreeFingerDragGesture(29, fields, 3));
  // Valid with unspecified touchCount (-1)
  QVERIFY(isThreeFingerDragGesture(29, fields, -1));

  // Valid during Changed
  fields.phase = GesturePhase::Changed;
  QVERIFY(isThreeFingerDragGesture(29, fields, 3));

  // Valid with digitizer hidType 11
  fields.hidType = 11;
  QVERIFY(isThreeFingerDragGesture(29, fields, 3));

  // Wrong touch count (1, 2, 4, 5) must be rejected
  QVERIFY(!isThreeFingerDragGesture(29, fields, 1));
  QVERIFY(!isThreeFingerDragGesture(29, fields, 2));
  QVERIFY(!isThreeFingerDragGesture(29, fields, 4));

  // Wrong event type
  fields.hidType = 27;
  QVERIFY(!isThreeFingerDragGesture(30, fields, 3));
  QVERIFY(!isThreeFingerDragGesture(1, fields, 3));

  // Dock swipe (swipeType 23) must never be treated as drag
  fields.swipeType = 23;
  QVERIFY(!isThreeFingerDragGesture(29, fields, 3));
  fields.swipeType = 0;

  // Axis-bound swipe (motionAxis != 0) must never be treated as drag
  fields.motionAxis = 1;
  QVERIFY(!isThreeFingerDragGesture(29, fields, 3));
  fields.motionAxis = 2;
  QVERIFY(!isThreeFingerDragGesture(29, fields, 3));
  fields.motionAxis = 0;

  // Terminal phases (Ended or Cancelled) must not be treated as active drag
  fields.phase = GesturePhase::Ended;
  QVERIFY(!isThreeFingerDragGesture(29, fields, 3));
  fields.phase = GesturePhase::Cancelled;
  QVERIFY(!isThreeFingerDragGesture(29, fields, 3));
}

void OSXGestureHelperTests::threeFingerDragTracker()
{
  OSXThreeFingerDragTracker tracker;

  // Initial state
  QVERIFY(!tracker.isDragging());
  QVERIFY(!tracker.hasLastPositionUnlocked());

  // First startDrag succeeds
  QVERIFY(tracker.startDrag());
  QVERIFY(tracker.isDragging());

  // Subsequent startDrag without cancel returns false
  QVERIFY(!tracker.startDrag());
  QVERIFY(tracker.isDragging());

  // Position tracking
  CGPoint pos1 = CGPointMake(100.0, 200.0);
  tracker.setLastPositionUnlocked(pos1);
  QVERIFY(tracker.hasLastPositionUnlocked());
  QCOMPARE(tracker.lastPositionUnlocked().x, 100.0);
  QCOMPARE(tracker.lastPositionUnlocked().y, 200.0);

  // Cancel drag succeeds
  QVERIFY(tracker.cancelDrag());
  QVERIFY(!tracker.isDragging());
  QVERIFY(!tracker.hasLastPositionUnlocked());

  // Subsequent cancelDrag returns false (idempotent)
  QVERIFY(!tracker.cancelDrag());
  QVERIFY(!tracker.isDragging());

  // Multithreaded concurrency test: ensure atomic transitions without race or deadlock
  std::atomic<int> startsObserved{0};
  std::atomic<int> cancelsObserved{0};
  constexpr int iterations = 1000;

  auto worker = [&]() {
    for (int i = 0; i < iterations; ++i) {
      if (tracker.startDrag()) {
        startsObserved++;
      }
      if (tracker.cancelDrag()) {
        cancelsObserved++;
      }
    }
  };

  std::thread t1(worker);
  std::thread t2(worker);
  t1.join();
  t2.join();

  // If still dragging at end, cancel it
  if (tracker.cancelDrag()) {
    cancelsObserved++;
  }

  QCOMPARE(startsObserved.load(), cancelsObserved.load());
  QVERIFY(!tracker.isDragging());
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

  // 2. Began -> Cancelled: resets state, sends nothing
  QVERIFY(!tracker.processEvent(began, dir));
  QVERIFY(tracker.isActive());

  OSXPrivateGestureFields cancelled;
  cancelled.phase = GesturePhase::Cancelled;
  QVERIFY(!tracker.processEvent(cancelled, dir));
  QVERIFY(!tracker.isActive());
  QCOMPARE(dir, OSXSwipeDirection::None);

  // 3. Changed -> Ended (valid swipe Up)
  QVERIFY(!tracker.processEvent(began, dir));

  OSXPrivateGestureFields changedUp;
  changedUp.phase = GesturePhase::Changed;
  changedUp.motionAxis = 2;
  changedUp.progress = 0.8;
  changedUp.velocity = 200.0;
  QVERIFY(!tracker.processEvent(changedUp, dir));
  QVERIFY(tracker.sawChanged());

  OSXPrivateGestureFields endedUp;
  endedUp.phase = GesturePhase::Ended;
  endedUp.motionAxis = 2;
  endedUp.progress = 1.0;
  endedUp.velocity = 0.0;
  QVERIFY(tracker.processEvent(endedUp, dir));
  QCOMPARE(dir, OSXSwipeDirection::Up);
  QVERIFY(!tracker.isActive());

  // 4. Duplicate Ended: must be ignored
  QVERIFY(!tracker.processEvent(endedUp, dir));

  // 5. Ended without Began: must be ignored
  tracker.reset();
  QVERIFY(!tracker.processEvent(endedUp, dir));
}

void OSXGestureHelperTests::nullEventSafe()
{
  OSXPrivateGestureFields fields = readPrivateGestureFields(nullptr);
  QCOMPARE(fields.hidType, 0LL);
  QCOMPARE(fields.swipeType, 0LL);
  QCOMPARE(fields.progress, 0.0);
  QCOMPARE(fields.velocity, 0.0);
  QCOMPARE(fields.phase, 0LL);
  QCOMPARE(fields.motionAxis, 0LL);
}

QTEST_MAIN(OSXGestureHelperTests)

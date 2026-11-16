/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2026 Deskflow Developers
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#pragma once

#include <QTest>

class OSXGestureHelperTests : public QObject
{
  Q_OBJECT

private Q_SLOTS:
  void classifyAllFourSwipeDirections();
  void classifyVelocityTieBreaker();
  void classifyInvalidAxisAndZeroMovement();
  void keystrokeMapping();
  void gesturePhasesHierarchy();
  void dockSwipeStateMachine();
  void nullEventSafe();
};

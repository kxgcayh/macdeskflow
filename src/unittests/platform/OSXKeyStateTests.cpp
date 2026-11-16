/*
 * Deskflow -- mouse and keyboard sharing utility
 * SPDX-FileCopyrightText: (C) 2025 Chris Rizzitello <sithlord48@gmail.com>
 * SPDX-FileCopyrightText: (C) 2012 - 2016 Synergy App Ltd
 * SPDX-FileCopyrightText: (C) 2011 Nick Bolton
 * SPDX-License-Identifier: GPL-2.0-only WITH LicenseRef-OpenSSL-Exception
 */

#include "OSXKeyStateTests.h"

#include "base/EventQueue.h"

#define SHIFT_ID_L kKeyShift_L
#define SHIFT_ID_R kKeyShift_R
#define SHIFT_BUTTON 57
#define A_CHAR_ID 0x00000061
#define A_CHAR_BUTTON 001

void OSXKeyStateTests::initTestCase()
{
  m_arch.init();
  m_log.setFilter(LogLevel::Level::Verbose);
}

void OSXKeyStateTests::mapModifiersFromOSX_OSXMask()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  KeyModifierMask outMask = 0;

  uint32_t shiftMask = 0 | kCGEventFlagMaskShift;
  outMask = keyState.mapModifiersFromOSX(shiftMask);
  QCOMPARE(outMask, KeyModifierShift);

  uint32_t ctrlMask = 0 | kCGEventFlagMaskControl;
  outMask = keyState.mapModifiersFromOSX(ctrlMask);
  QCOMPARE(outMask, KeyModifierControl);

  uint32_t altMask = 0 | kCGEventFlagMaskAlternate;
  outMask = keyState.mapModifiersFromOSX(altMask);
  QCOMPARE(outMask, KeyModifierAlt);

  uint32_t cmdMask = 0 | kCGEventFlagMaskCommand;
  outMask = keyState.mapModifiersFromOSX(cmdMask);
  QCOMPARE(outMask, KeyModifierSuper);

  uint32_t capsMask = 0 | kCGEventFlagMaskAlphaShift;
  outMask = keyState.mapModifiersFromOSX(capsMask);
  QCOMPARE(outMask, KeyModifierCapsLock);

  uint32_t numMask = 0 | kCGEventFlagMaskNumericPad;
  outMask = keyState.mapModifiersFromOSX(numMask);
  QCOMPARE(outMask, KeyModifierNumLock);
}

void OSXKeyStateTests::syntheticFlagsForVirtualKeys()
{
  const CGEventFlags navExpected = kCGEventFlagMaskNumericPad | kCGEventFlagMaskSecondaryFn;
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_LeftArrow), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_RightArrow), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_DownArrow), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_UpArrow), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_Home), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_End), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_PageUp), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_PageDown), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_Help), navExpected);
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_ForwardDelete), navExpected);

  const CGEventFlags fnExpected = kCGEventFlagMaskSecondaryFn;
  const CGKeyCode fnKeys[] = {kVK_F1,  kVK_F2,  kVK_F3,  kVK_F4,  kVK_F5,  kVK_F6,  kVK_F7,  kVK_F8,  kVK_F9,  kVK_F10,
                              kVK_F11, kVK_F12, kVK_F13, kVK_F14, kVK_F15, kVK_F16, kVK_F17, kVK_F18, kVK_F19, kVK_F20};
  for (CGKeyCode vk : fnKeys) {
    QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(vk), fnExpected);
  }

  // Mission Control VK (160)
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(160), fnExpected);

  // Standard alphanumeric keys should carry no synthetic flags
  QCOMPARE(OSXKeyState::getSyntheticFlagsForVirtualKey(kVK_ANSI_A), 0ULL);
}

void OSXKeyStateTests::virtualKeyFlags()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);

  // Initially without modifiers
  QCOMPARE(keyState.getKeyboardEventFlags(), 0ULL);
  QCOMPARE(keyState.getVirtualKeyFlags(kVK_ANSI_A), 0ULL);
  QCOMPARE(
      keyState.getVirtualKeyFlags(kVK_UpArrow),
      static_cast<CGEventFlags>(kCGEventFlagMaskNumericPad | kCGEventFlagMaskSecondaryFn)
  );

  // Set control modifier
  keyState.setKeyboardModifiers(kVK_Control, true);
  const CGEventFlags ctrlExpected = keyState.getKeyboardEventFlags();
  QVERIFY((ctrlExpected & kCGEventFlagMaskControl) != 0);

  // Ctrl+Up should carry control + numeric pad + secondary fn
  const CGEventFlags ctrlUpFlags = keyState.getVirtualKeyFlags(kVK_UpArrow);
  QCOMPARE(ctrlUpFlags, ctrlExpected | kCGEventFlagMaskNumericPad | kCGEventFlagMaskSecondaryFn);

  // Release control modifier: ensure flags do not leak into modifier state or other keys
  keyState.setKeyboardModifiers(kVK_Control, false);
  QCOMPARE(keyState.getKeyboardEventFlags(), 0ULL);
  QCOMPARE(keyState.getVirtualKeyFlags(kVK_ANSI_A), 0ULL);
}

void OSXKeyStateTests::fakePollShift()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(SHIFT_ID_L, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyDown(SHIFT_ID_R, 0, 2, "en");
  QVERIFY(isKeyPressed(keyState, SHIFT_BUTTON));

  keyState.fakeKeyUp(2);
  QVERIFY(!isKeyPressed(keyState, SHIFT_BUTTON));
}

void OSXKeyStateTests::fakePollChar()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, 0, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

void OSXKeyStateTests::fakePollCharWithModifier()
{
  deskflow::KeyMap keyMap;
  EventQueue eventQueue;
  OSXKeyState keyState(&eventQueue, keyMap, {"en"}, true);
  keyState.updateKeyMap();

  keyState.fakeKeyDown(A_CHAR_ID, KeyModifierShift, 1, "en");
  QVERIFY(isKeyPressed(keyState, A_CHAR_BUTTON));

  keyState.fakeKeyUp(1);
  QVERIFY(!isKeyPressed(keyState, A_CHAR_BUTTON));

  // HACK: delete the key in case it was typed into a text editor.
  // we should really set focus to an invisible window.
  keyState.fakeKeyDown(kKeyBackSpace, 0, 2, "en");
  keyState.fakeKeyUp(2);
}

bool OSXKeyStateTests::isKeyPressed(const OSXKeyState &keyState, KeyButton button)
{
  // HACK: allow os to realize key state changes.
  Arch::sleep(.2);

  IKeyState::KeyButtonSet pressed;
  keyState.pollPressedKeys(pressed);

  IKeyState::KeyButtonSet::const_iterator it;
  for (it = pressed.begin(); it != pressed.end(); ++it) {
    LOG_DEBUG("checking key %d", *it);
    if (*it == button) {
      return true;
    }
  }
  return false;
}

QTEST_MAIN(OSXKeyStateTests)

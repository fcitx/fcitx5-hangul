/*
 * SPDX-FileCopyrightText: 2024-2024 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 */
#include "testdir.h"
#include "testfrontend_public.h"
#include <fcitx-utils/eventdispatcher.h>
#include <fcitx-utils/key.h>
#include <fcitx-utils/keysym.h>
#include <fcitx-utils/log.h>
#include <fcitx-utils/macros.h>
#include <fcitx-utils/testing.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputmethodgroup.h>
#include <fcitx/inputmethodmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/instance.h>

using namespace fcitx;

void scheduleEvent(Instance *instance) {
    instance->eventDispatcher().schedule([instance]() {
        auto *hangul = instance->addonManager().addon("hangul", true);
        FCITX_ASSERT(hangul);
        auto defaultGroup = instance->inputMethodManager().currentGroup();
        defaultGroup.inputMethodList().clear();
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("keyboard-us"));
        defaultGroup.inputMethodList().push_back(
            InputMethodGroupItem("hangul"));
        defaultGroup.setDefaultInputMethod("");
        instance->inputMethodManager().setGroup(defaultGroup);
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid =
            testfrontend->call<ITestFrontend::createInputContext>("testapp");
        auto *ic = instance->inputContextManager().findByUUID(uuid);
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Control+space"), false));

        testfrontend->call<ITestFrontend::pushCommitExpectation>("ㅂ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ㅃ");
        testfrontend->call<ITestFrontend::pushCommitExpectation>("ㅂ");
        FCITX_ASSERT(instance->inputMethod(ic) == "hangul");

        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("q"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Q"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_Q, KeyState::CapsLock), false));
        instance->deactivate();
    });

    // Backspace propagation test.
    // Expectation per requirements.md: while composing, backspace clears the
    // composition jamo-by-jamo and is consumed by the IME; once the buffer is
    // empty the next backspace should propagate to the application so that
    // press-and-hold seamlessly continues deleting previously committed text.
    instance->eventDispatcher().schedule([instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid = testfrontend->call<ITestFrontend::createInputContext>(
            "backspace-test");
        auto *ic = instance->inputContextManager().findByUUID(uuid);

        // Activate hangul on this fresh context.
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Control+space"), false));
        FCITX_ASSERT(instance->inputMethod(ic) == "hangul");

        // No composition yet → backspace must propagate (not consumed).
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        // Type "rk" → 가 in dubeolsik (ㄱ + ㅏ).
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("r"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("k"), false));

        // Backspace 1: 가 → ㄱ, consumed.
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        // Backspace 2: ㄱ → empty, consumed.
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        // Backspace 3: empty buffer → must propagate.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        // Type a 3-jamo syllable "gks" → 한 (ㅎ + ㅏ + ㄴ).
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("g"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("k"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("s"), false));
        // Need 3 backspaces to empty: 한 → 하 → ㅎ → empty.
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        // 4th propagates.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        instance->deactivate();
    });

    // Typing-then-backspace: commit something to the app, then test that
    // backspace immediately propagates so the app can delete it. This is the
    // exact "press-and-hold past the buffer" scenario in requirements.md.
    instance->eventDispatcher().schedule([instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid = testfrontend->call<ITestFrontend::createInputContext>(
            "commit-then-backspace");
        auto *ic = instance->inputContextManager().findByUUID(uuid);

        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Control+space"), false));
        FCITX_ASSERT(instance->inputMethod(ic) == "hangul");

        // Type "rk" → 가 in composition. Then any non-hangul key forces a
        // flush, committing 가 to the app.
        testfrontend->call<ITestFrontend::pushCommitExpectation>("가");
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("r"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("k"), false));
        // 1 (digit) is not a Korean jamo; engine flushes 가, propagates the 1.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("1"), false));

        // Now composition is empty and 가 was committed. Backspace must
        // propagate so the app can delete it.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        instance->deactivate();
    });

    // Repeat-on-empty-buffer test. Regression for the press-and-hold bug
    // where Wayland frontends drop synthesized RELEASE/PRESS pairs for
    // auto-repeated backspace events. The engine works around this by
    // explicitly forwarding the key (which consumes the event) when the
    // composition buffer is already empty and the event is a repeat. A single
    // (non-repeat) press on the same empty buffer must still propagate
    // naturally so unrelated edge cases keep working.
    instance->eventDispatcher().schedule([instance]() {
        auto *testfrontend = instance->addonManager().addon("testfrontend");
        auto uuid = testfrontend->call<ITestFrontend::createInputContext>(
            "backspace-repeat");
        auto *ic = instance->inputContextManager().findByUUID(uuid);

        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("Control+space"), false));
        FCITX_ASSERT(instance->inputMethod(ic) == "hangul");

        // Single press on empty composition: must propagate (consumed=false).
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        // Auto-repeat press on empty composition: must be consumed by the
        // workaround so the engine can forwardKey on the app's behalf.
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace, KeyState::Repeat), false));

        // While composition is non-empty, both single and repeat presses
        // should still consume normally (clearing one jamo each).
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("r"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key("k"), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace, KeyState::Repeat), false));
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));
        // Buffer is now empty again. A repeat must consume (workaround).
        FCITX_ASSERT(testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace, KeyState::Repeat), false));
        // And a single press at the same point must still propagate.
        FCITX_ASSERT(!testfrontend->call<ITestFrontend::sendKeyEvent>(
            uuid, Key(FcitxKey_BackSpace), false));

        instance->deactivate();
    });

    instance->eventDispatcher().schedule([instance]() { instance->exit(); });
}

int main() {
    setupTestingEnvironmentPath(TESTING_BINARY_DIR, {"bin"},
                                {TESTING_BINARY_DIR "/test"});
    char arg0[] = "testhangul";
    char arg1[] = "--disable=all";
    char arg2[] = "--enable=testim,testfrontend,hangul";
    char *argv[] = {arg0, arg1, arg2};
    fcitx::Log::setLogRule("default=5,hangul=5");
    Instance instance(FCITX_ARRAY_SIZE(argv), argv);
    instance.addonManager().registerDefaultLoader(nullptr);
    scheduleEvent(&instance);
    instance.exec();

    return 0;
}

/*
 * SPDX-FileCopyrightText: 2010~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */

#include "engine.h"
#include <fcitx-utils/standardpath.h>
#include <fcitx-utils/utf8.h>
#include <fcitx/action.h>
#include <fcitx/candidatelist.h>
#include <fcitx/inputcontext.h>
#include <fcitx/inputcontextmanager.h>
#include <fcitx/inputpanel.h>
#include <fcitx/statusarea.h>
#include <fcitx/userinterfacemanager.h>

static const char *keyboardId[] = {"2",  "2y", "39", "3f", "3s",
                                   "3y", "32", "ro", "ahn"};

constexpr auto MAX_LENGTH = 40;

namespace fcitx {

const KeyList &selectionKeys() {
    static const KeyList selectionKeys{
        Key(FcitxKey_1), Key(FcitxKey_2), Key(FcitxKey_3), Key(FcitxKey_4),
        Key(FcitxKey_5), Key(FcitxKey_6), Key(FcitxKey_7), Key(FcitxKey_8),
        Key(FcitxKey_9), Key(FcitxKey_0)};
    return selectionKeys;
}

std::string ustringToUTF8(const UString &ustr) {
    std::string result;
    for (auto c : ustr) {
        result += utf8::UCS4ToUTF8(c);
    }
    return result;
}

static std::string subUTF8String(const std::string &str, int p1, int p2) {
    int limit;
    int pos;
    int n;

    if (str.empty()) {
        return "";
    }

    limit = str.size() + 1;

    p1 = std::max(0, p1);
    p2 = std::max(0, p2);

    pos = std::min(p1, p2);
    n = std::abs(p2 - p1);

    if (pos + n > limit) {
        n = limit - pos;
    }

    auto begin = utf8::nextNChar(str.begin(), pos);
    auto end = utf8::nextNChar(begin, n);

    return std::string(begin, end);
}

class HangulCandidate : public CandidateWord {
public:
    HangulCandidate(HangulEngine *engine, int idx, std::string text)
        : engine_(engine), idx_(idx) {
        setText(Text(std::move(text)));
    }

    void select(InputContext *inputContext) const override;

private:
    HangulEngine *engine_;
    int idx_;
};

class HangulState : public InputContextProperty {
public:
    HangulState(HangulEngine *engine, InputContext *ic)
        : engine_(engine), ic_(ic) {
        configure();
    }

    void configure() {
        context_.reset(hangul_ic_new(
            keyboardId[static_cast<int>(*engine_->config().keyboard)]));
        hangul_ic_connect_callback(
            context_.get(), "transition",
            reinterpret_cast<void *>(&HangulState::onTransitionCallback), this);
    }

    static bool onTransitionCallback(HangulInputContext *, ucschar c,
                                     const ucschar *, void *data) {
        auto that = static_cast<HangulState *>(data);
        return that->onTransition(c);
    }

    bool onTransition(ucschar c) {
        if (!*engine_->config().autoReorder) {
            if (hangul_is_choseong(c)) {
                if (hangul_ic_has_jungseong(context_.get()) ||
                    hangul_ic_has_jongseong(context_.get()))
                    return false;
            }

            if (hangul_is_jungseong(c)) {
                if (hangul_ic_has_jongseong(context_.get()))
                    return false;
            }
        }

        return true;
    }

    void updateLookupTable(bool checkSurrounding) {
        std::string hanjaKey;
        LookupMethod lookupMethod = LOOKUP_METHOD_PREFIX;

        hanjaList_.reset();

        auto hic_preedit = hangul_ic_get_preedit_string(context_.get());
        UString preedit = preedit_;
        preedit.append(UString(hic_preedit));
        if (!preedit.empty()) {
            auto utf8 = ustringToUTF8(preedit);
            if (*engine_->config().wordCommit || *engine_->config().hanjaMode) {
                hanjaKey = utf8;
                lookupMethod = LOOKUP_METHOD_PREFIX;
            } else {
                auto cursorPos = ic_->surroundingText().cursor();
                auto substr =
                    subUTF8String(ic_->surroundingText().text(),
                                  static_cast<int>(cursorPos) - 64, cursorPos);

                if (!substr.empty()) {
                    hanjaKey = substr + utf8;
                } else {
                    hanjaKey = utf8;
                }
                lookupMethod = LOOKUP_METHOD_SUFFIX;
            }
        } else if (checkSurrounding) {

            if (!ic_->capabilityFlags().test(CapabilityFlag::SurroundingText) ||
                !ic_->surroundingText().isValid()) {
                return;
            }
            auto surroundingStr = ic_->surroundingText().text();
            auto cursorPos = ic_->surroundingText().cursor();
            auto anchorPos = ic_->surroundingText().anchor();
            if (cursorPos != anchorPos) {
                // If we have selection in surrounding text, we use that.
                hanjaKey = subUTF8String(surroundingStr, cursorPos, anchorPos);
                lookupMethod = LOOKUP_METHOD_EXACT;
            } else {
                hanjaKey =
                    subUTF8String(surroundingStr,
                                  static_cast<int>(cursorPos) - 64, cursorPos);
                lookupMethod = LOOKUP_METHOD_SUFFIX;
            }
        }

        if (!hanjaKey.empty()) {
            hanjaList_.reset(lookupTable(hanjaKey, lookupMethod));
            lastLookupMethod_ = lookupMethod;
        }
    }

    HanjaList *lookupTable(const std::string key, int method) {
        HanjaList *list = nullptr;

        if (key.empty())
            return nullptr;

        decltype(&hanja_table_match_exact) func = nullptr;

        switch (method) {
        case LOOKUP_METHOD_EXACT:
            func = &hanja_table_match_exact;
            break;
        case LOOKUP_METHOD_PREFIX:
            func = &hanja_table_match_prefix;
            break;
        case LOOKUP_METHOD_SUFFIX:
            func = &hanja_table_match_suffix;
            break;
        }
        if (!func) {
            return nullptr;
        }

        if (auto symbolTable = engine_->symbolTable()) {
            list = func(symbolTable, key.data());
        }

        if (!list) {
            list = func(engine_->table(), key.data());
        }

        return list;
    }

    void keyEvent(KeyEvent &keyEvent) {
        if (keyEvent.isRelease()) {
            return;
        }

        if (keyEvent.key().checkKeyList(
                *engine_->config().hanjaModeToggleKey)) {
            if (!hanjaList_) {
                updateLookupTable(true);
            } else {
                cleanup();
            }
            updateUI();
            return keyEvent.filterAndAccept();
        }

        auto sym = keyEvent.key().sym();

        if (sym == FcitxKey_Shift_L || sym == FcitxKey_Shift_R) {
            return;
        }

        KeyStates s;
        for (auto keyList :
             {&*engine_->config().hanjaModeToggleKey,
              &*engine_->config().prevPageKey, &*engine_->config().nextPageKey,
              &*engine_->config().prevCandidateKey,
              &*engine_->config().nextCandidateKey}) {
            for (auto key : *keyList) {
                s |= key.states();
            }
        }

        struct {
            KeyState state;
            KeySym left, right;
        } modifiers[] = {
            {KeyState::Ctrl, FcitxKey_Control_L, FcitxKey_Control_R},
            {KeyState::Alt, FcitxKey_Alt_L, FcitxKey_Alt_R},
            {KeyState::Shift, FcitxKey_Shift_L, FcitxKey_Shift_R},
            {KeyState::Super, FcitxKey_Super_L, FcitxKey_Super_R},
            {KeyState::Hyper, FcitxKey_Hyper_L, FcitxKey_Hyper_R},
        };

        for (auto &modifier : modifiers) {
            if (s & modifier.state) {
                if (sym == modifier.left || sym == modifier.right) {
                    return;
                }
            }
        }

        // Handle candidate selection.
        auto candList = ic_->inputPanel().candidateList();
        if (candList && candList->size() > 0) {
            if (keyEvent.key().checkKeyList(*engine_->config().prevPageKey)) {
                candList->toPageable()->prev();
                ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
                return keyEvent.filterAndAccept();
            } else if (keyEvent.key().checkKeyList(
                           *engine_->config().nextPageKey)) {
                candList->toPageable()->next();
                ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
                return keyEvent.filterAndAccept();
            }

            if (keyEvent.key().checkKeyList(
                    *engine_->config().prevCandidateKey)) {
                candList->toCursorMovable()->prevCandidate();
                ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
                return keyEvent.filterAndAccept();
            } else if (keyEvent.key().checkKeyList(
                           *engine_->config().nextCandidateKey)) {
                candList->toCursorMovable()->nextCandidate();
                ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
                return keyEvent.filterAndAccept();
            }

            auto idx = keyEvent.key().keyListIndex(selectionKeys());
            if (idx >= 0) {
                if (idx < candList->size()) {
                    candList->candidate(idx).select(ic_);
                }
                return keyEvent.filterAndAccept();
            }

            if (keyEvent.key().check(FcitxKey_Return)) {
                auto idx = candList->cursorIndex();
                if (idx < 0) {
                    idx = 0;
                }

                if (idx < candList->size()) {
                    candList->candidate(idx).select(ic_);
                    return keyEvent.filterAndAccept();
                }
            }

            if (!*engine_->config().hanjaMode) {
                cleanup();
            }
        }

        s = KeyStates{KeyState::Ctrl, KeyState::Alt, KeyState::Shift,
                      KeyState::Super, KeyState::Hyper};
        if (keyEvent.key().states() & s) {
            flush();
            updateUI();
            return;
        }

        bool keyUsed = false;
        if (keyEvent.key().check(FcitxKey_BackSpace)) {
            keyUsed = hangul_ic_backspace(context_.get());
            if (!keyUsed) {
                unsigned int preedit_len = preedit_.size();
                if (preedit_len > 0) {
                    preedit_.pop_back();
                    keyUsed = true;
                }
            }
        } else {
            if (preedit_.size() >= MAX_LENGTH) {
                flush();
            }

            keyUsed = hangul_ic_process(context_.get(), keyEvent.key().sym());
            bool notFlush = false;

            const ucschar *str = hangul_ic_get_commit_string(context_.get());
            if (*engine_->config().wordCommit || *engine_->config().hanjaMode) {
                const ucschar *hic_preedit;

                hic_preedit = hangul_ic_get_preedit_string(context_.get());
                if (hic_preedit != NULL && hic_preedit[0] != 0) {
                    preedit_.append(UString(str));
                } else {
                    preedit_.append(UString(str));
                    if (!preedit_.empty()) {
                        auto commit = ustringToUTF8(preedit_);
                        if (!commit.empty()) {
                            ic_->commitString(commit);
                        }
                    }
                    preedit_.clear();
                }
            } else {
                if (str != NULL && str[0] != 0) {
                    auto commit = ustringToUTF8(str);
                    if (!commit.empty()) {
                        ic_->commitString(commit);
                    }
                }
            }

            if (!keyUsed && !notFlush) {
                flush();
            }
        }

        if (*engine_->config().hanjaMode) {
            updateLookupTable(false);
        } else {
            cleanup();
        }

        updateUI();
        if (keyUsed) {
            keyEvent.filterAndAccept();
        }
    }

    void reset() {
        preedit_.clear();
        hangul_ic_reset(context_.get());
        hanjaList_.reset();
        updateUI();
    }

    void cleanup() { hanjaList_.reset(); }

    void flush() {
        cleanup();

        auto str = hangul_ic_flush(context_.get());

        preedit_ += str;

        if (preedit_.empty())
            return;

        auto utf8 = ustringToUTF8(preedit_);
        if (!utf8.empty()) {
            ic_->commitString(utf8);
        }

        preedit_.clear();
    }

    void updateUI() {
        const ucschar *hic_preedit =
            hangul_ic_get_preedit_string(context_.get());

        ic_->inputPanel().reset();

        auto pre1 = ustringToUTF8(preedit_);
        auto pre2 = hic_preedit ? ustringToUTF8(hic_preedit) : "";

        if (!pre1.empty() || !pre2.empty()) {
            Text text;
            text.append(pre1);
            text.append(pre2, TextFormatFlag::HighLight);
            text.setCursor(pre1.size() + pre2.size());
            if (ic_->capabilityFlags().test(CapabilityFlag::Preedit)) {
                ic_->inputPanel().setClientPreedit(text);
            } else {
                ic_->inputPanel().setPreedit(text);
            }
        }
        ic_->updatePreedit();

        setLookupTable();

        ic_->updateUserInterface(UserInterfaceComponent::InputPanel);
    }

    void setLookupTable() {
        if (!hanjaList_) {
            return;
        }
        HanjaList *list = hanjaList_.get();
        if (list) {
            auto candidate = std::make_unique<CommonCandidateList>();
            candidate->setSelectionKey(selectionKeys());
            candidate->setCursorPositionAfterPaging(
                CursorPositionAfterPaging::ResetToFirst);
            candidate->setPageSize(
                engine_->instance()->globalConfig().defaultPageSize());
            auto n = hanja_list_get_size(list);
            for (auto i = 0; i < n; i++) {
                const char *value = hanja_list_get_nth_value(list, i);
                candidate->append<HangulCandidate>(engine_, i, value);
            }
            if (n) {
                candidate->setGlobalCursorIndex(0);
                ic_->inputPanel().setCandidateList(std::move(candidate));
            }
        }
    }

    void select(int pos) {
        const char *key;
        const char *value;
        const ucschar *hic_preedit;
        int key_len;
        int preedit_len;
        int hic_preedit_len;

        key = hanja_list_get_nth_key(hanjaList_.get(), pos);
        value = hanja_list_get_nth_value(hanjaList_.get(), pos);
        hic_preedit = hangul_ic_get_preedit_string(context_.get());

        if (!key || !value || !hic_preedit) {
            return reset();
        }

        key_len = fcitx::utf8::length(std::string(key));
        preedit_len = preedit_.size();
        hic_preedit_len = UString(hic_preedit).size();

        bool surrounding = false;
        if (lastLookupMethod_ == LOOKUP_METHOD_PREFIX) {
            if (preedit_len == 0 && hic_preedit_len == 0) {
                /* remove surrounding_text */
                if (key_len > 0) {
                    ic_->deleteSurroundingText(-key_len, key_len);
                    surrounding = true;
                }
            } else {
                /* remove preedit text */
                if (key_len > 0) {
                    long n = std::min(key_len, preedit_len);
                    preedit_.erase(0, n);
                    key_len -= preedit_len;
                }

                /* remove hic preedit text */
                if (key_len > 0) {
                    hangul_ic_reset(context_.get());
                    key_len -= hic_preedit_len;
                }
            }
        } else {
            /* remove hic preedit text */
            if (hic_preedit_len > 0) {
                hangul_ic_reset(context_.get());
                key_len -= hic_preedit_len;
            }

            /* remove preedit text */
            if (key_len > preedit_len) {
                preedit_.erase(0, preedit_len);
                key_len -= preedit_len;
            } else if (key_len > 0) {
                preedit_.erase(0, key_len);
                key_len = 0;
            }

            /* remove surrounding_text */
            if (LOOKUP_METHOD_EXACT != lastLookupMethod_ && key_len > 0) {
                ic_->deleteSurroundingText(-key_len, key_len);
                surrounding = true;
            }
        }

        ic_->commitString(value);
        if (surrounding) {
            cleanup();
        }
        updateLookupTable(false);
        updateUI();
    }

private:
    HangulEngine *engine_;
    InputContext *ic_;
    UniqueCPtr<HangulInputContext, &hangul_ic_delete> context_;
    UniqueCPtr<HanjaList, &hanja_list_delete> hanjaList_;
    UString preedit_;
    LookupMethod lastLookupMethod_;
};

HangulEngine::HangulEngine(Instance *instance)
    : instance_(instance),
      factory_([this](InputContext &ic) { return new HangulState(this, &ic); }),
      table_(hanja_table_load(nullptr)) {
    if (!table_) {
        throw std::runtime_error("Failed to load hanja table.");
    }

    auto file = StandardPath::global().locate(StandardPath::Type::PkgData,
                                              "hangul/symbol.txt");
    if (!file.empty()) {
        symbolTable_.reset(hanja_table_load(file.data()));
    }

    reloadConfig();
    action_.connect<SimpleAction::Activated>([this](InputContext *ic) {
        config_.hanjaMode.setValue(!*config_.hanjaMode);
        updateAction(ic);
    });
    instance_->userInterfaceManager().registerAction("hangul", &action_);

    instance_->inputContextManager().registerProperty("hangulState", &factory_);
}

void HangulEngine::activate(const InputMethodEntry &,
                            InputContextEvent &event) {
    event.inputContext()->statusArea().addAction(StatusGroup::InputMethod,
                                                 &action_);
    updateAction(event.inputContext());
}

void HangulEngine::deactivate(const InputMethodEntry &entry,
                              InputContextEvent &event) {
    if (event.type() == EventType::InputContextSwitchInputMethod) {
        auto state = event.inputContext()->propertyFor(&factory_);
        state->flush();
    }
    reset(entry, event);
}

void HangulEngine::keyEvent(const InputMethodEntry &, KeyEvent &keyEvent) {
    if (keyEvent.isRelease()) {
        return;
    }
    auto state = keyEvent.inputContext()->propertyFor(&factory_);
    state->keyEvent(keyEvent);
}

void HangulEngine::reset(const InputMethodEntry &, InputContextEvent &event) {
    auto state = event.inputContext()->propertyFor(&factory_);
    state->reset();
}

void HangulEngine::reloadConfig() { readAsIni(config_, "conf/hangul.conf"); }

HangulState *HangulEngine::state(InputContext *ic) {
    return ic->propertyFor(&factory_);
}

void HangulCandidate::select(InputContext *inputContext) const {
    auto state = engine_->state(inputContext);
    state->select(idx_);
}
} // namespace fcitx

FCITX_ADDON_FACTORY(fcitx::HangulEngineFactory);

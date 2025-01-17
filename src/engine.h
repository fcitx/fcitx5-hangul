/*
 * SPDX-FileCopyrightText: 2010~2021 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 */
#ifndef _FCITX5_HANGUL_ENGINE_H_
#define _FCITX5_HANGUL_ENGINE_H_

#include <fcitx-config/iniparser.h>
#include <fcitx-utils/i18n.h>
#include <fcitx/action.h>
#include <fcitx/addonfactory.h>
#include <fcitx/addonmanager.h>
#include <fcitx/inputcontextproperty.h>
#include <fcitx/inputmethodengine.h>
#include <fcitx/instance.h>
#include <hangul.h>
#include <string>

namespace fcitx {

enum class HangulKeyboard {
    Dubeolsik = 0,
    Dubeolsik_Yetgeul,
    Sebeolsik_390,
    Sebeolsik_Final,
    Sebeolsik_Noshift,
    Sebeolsik_Yetgeul,
    Sebeolsik_Dubeol_Layout,
    Romaja,
    Ahnmatae,
};

FCITX_CONFIG_ENUM_NAME_WITH_I18N(HangulKeyboard, N_("Dubeolsik"),
                                 N_("Dubeolsik Yetgeul"), N_("Sebeolsik 390"),
                                 N_("Sebeolsik Final"), N_("Sebeolsik Noshift"),
                                 N_("Sebeolsik Yetgeul"),
                                 N_("Sebeolsik Dubeol Layout"), N_("Romaja"),
                                 N_("Ahnmatae"));

FCITX_CONFIGURATION(
    HangulConfig,
    OptionWithAnnotation<HangulKeyboard, HangulKeyboardI18NAnnotation> keyboard{
        this, "Keyboard", _("Keyboard Layout"), HangulKeyboard::Dubeolsik};
    KeyListOption hanjaModeToggleKey{
        this,
        "HanjaModeToggleKey",
        _("Hanja Mode Toggle Key"),
        {Key(FcitxKey_Hangul_Hanja), Key(FcitxKey_F9)},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption prevPageKey{
        this,
        "PrevPage",
        _("Prev Page"),
        {Key(FcitxKey_Up)},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption nextPageKey{
        this,
        "NextPage",
        _("Next Page"),
        {Key(FcitxKey_Down)},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption prevCandidateKey{
        this,
        "PrevCandidate",
        _("Prev Candidate"),
        {Key(FcitxKey_Tab, KeyState::Shift)},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    KeyListOption nextCandidateKey{
        this,
        "NextCandidate",
        _("Next Candidate"),
        {Key(FcitxKey_Tab)},
        KeyListConstrain(KeyConstrainFlag::AllowModifierLess)};
    Option<bool> autoReorder{this, "AutoReorder", _("Auto Reorder"), true};
    Option<bool> wordCommit{this, "WordCommit", _("Word Commit"), false};
    Option<bool> hanjaMode{this, "HanjaMode", _("Hanja Mode"), false};);

typedef enum _LookupMethod {
    LOOKUP_METHOD_PREFIX,
    LOOKUP_METHOD_EXACT,
    LOOKUP_METHOD_SUFFIX
} LookupMethod;

class HangulState;

using UString = std::basic_string<char32_t>;

UString ucsToUString(const ucschar *c);

class HangulEngine : public InputMethodEngine {
public:
    HangulEngine(Instance *instance);

    void activate(const fcitx::InputMethodEntry &,
                  fcitx::InputContextEvent &) override;
    void deactivate(const fcitx::InputMethodEntry &entry,
                    fcitx::InputContextEvent &event) override;
    void keyEvent(const fcitx::InputMethodEntry &entry,
                  fcitx::KeyEvent &keyEvent) override;
    void reset(const fcitx::InputMethodEntry &,
               fcitx::InputContextEvent &) override;
    void reloadConfig() override;

    const fcitx::Configuration *getConfig() const override { return &config_; }

    void setConfig(const fcitx::RawConfig &rawConfig) override;

    auto &config() { return config_; }

    auto table() { return table_.get(); }
    auto symbolTable() { return symbolTable_.get(); }

    HangulState *state(InputContext *ic);

    void updateAction(InputContext *ic) {
        action_.setIcon(*config_.hanjaMode ? "fcitx-hanja-active"
                                           : "fcitx-hanja-inactive");
        action_.setLongText(*config_.hanjaMode ? _("Use Hanja")
                                               : _("Use Hangul"));
        action_.setShortText(*config_.hanjaMode ? "\xe9\x9f\x93"
                                                : "\xed\x95\x9c");
        action_.update(ic);
        safeSaveAsIni(config_, "conf/hangul.conf");
    }

    auto instance() { return instance_; }

private:
    Instance *instance_;
    HangulConfig config_;
    FactoryFor<HangulState> factory_;
    UniqueCPtr<HanjaTable, hanja_table_delete> table_;
    UniqueCPtr<HanjaTable, hanja_table_delete> symbolTable_;
    SimpleAction action_;
};

class HangulEngineFactory : public AddonFactory {
public:
    AddonInstance *create(AddonManager *manager) override {
        registerDomain("fcitx5-hangul", FCITX_INSTALL_LOCALEDIR);
        return new HangulEngine(manager->instance());
    }
};
} // namespace fcitx

#endif // _FCITX5_HANGUL_ENGINE_H_

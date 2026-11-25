#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include "hooks/EditorUI.hpp"

using namespace geode::prelude;

class HotbarMenuPopup : public geode::Popup {
protected:
    ScrollLayer *m_scrollLayer = nullptr;

    bool init() override;
    void setupList();
    void onDeletePage(CCObject *sender);
    void onAddPage(CCObject *sender);
    void onOpenModSettings(CCObject *sender);

public:
    static HotbarMenuPopup *create();
};
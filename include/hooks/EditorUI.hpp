#pragma once

#include <Geode/Geode.hpp>
#include <Geode/modify/EditorUI.hpp>
#include <Geode/modify/GameManager.hpp>
#include <Geode/ui/GeodeUI.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <alphalaneous.tinker/UIScaling.hpp>

using namespace geode::prelude;

class HotbarMenuPopup;

class $modify(ModEditorUI, EditorUI) {
    struct Fields {
        std::vector<std::array<int, 10>> m_hotbarPages;
        int m_currentPage = 0;
        int m_assigningPage = -1;
        int m_assigningSlot = -1;
        bool m_uiVisible = true;
    };
    
    geode::NineSlice *createHotbarLayout(bool inPopup = false);
    bool init(LevelEditorLayer *editorLayer);
    void onHotbarMenu(CCObject *sender);
    void onPageLeft(CCObject *sender);
    void onPageRight(CCObject *sender);
    void addPage();
    void removePage(int index);
    int getSlotObject(int slot);
    void setSlotObject(int slot, int value, int page = -1);
    void loadHotbar();
    void saveHotbar();
    void applySelectionTint(CCNode *node);

    void updateHotbar(bool triggeredByCreateMenuSelect = false);

    void updateHotbarSlot(int slot, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn);

    void updateFilledSlot(int slot, int objectID, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn);

    void updateEmptySlot(int slot, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn);

    void onCreateButton(CCObject *sender);
    void onObjectSelected(CCObject *sender);
    void onObjectRemoved(CCObject *sender);
    void toggleMode(CCObject *sender);
    void showUI(bool show);
};

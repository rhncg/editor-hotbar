#include "hooks/EditorUI.hpp"
#include "ui/HotbarMenuPopup.hpp"
#include <asp/fs/std.hpp>

geode::NineSlice *ModEditorUI::createHotbarLayout(bool inPopup) {
    auto hotbar = geode::NineSlice::create("square02b_001.png");
    hotbar->setContentSize({245, 30});

    if (Mod::get()->getSettingValue<bool>("show-hotbar-outline")) {
        auto outline = geode::NineSlice::create("GJ_square07.png");
        outline->setContentSize({245, 30});
        outline->setPosition({245 / 2.0f, 30 / 2.0f});
        hotbar->addChild(outline);
    }
    if (!inPopup)
        hotbar->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

    hotbar->setColor(Mod::get()->getSettingValue<cocos2d::ccColor3B>("hotbar-color"));

    auto hotbarMenu = CCMenu::create();
    hotbarMenu->setContentSize(hotbar->getContentSize());
    hotbarMenu->setPosition(hotbar->getContentSize() / 2.0f);

    auto layout = AxisLayout::create(Axis::Row)
                      ->setGap(5.0f)
                      ->setGrowCrossAxis(true)
                      ->setCrossAxisOverflow(false)
                      ->setAxisAlignment(AxisAlignment::Center)
                      ->setCrossAxisAlignment(AxisAlignment::Center)
                      ->setAutoScale(true)
                      ->setPadding({5, 0, 5, 0});

    hotbarMenu->setLayout(layout);

    CCObject *target = inPopup ? nullptr : this;
    auto selectCallback = inPopup ? nullptr : menu_selector(ModEditorUI::onObjectSelected);
    auto removeCallback = inPopup ? nullptr : menu_selector(ModEditorUI::onObjectRemoved);

    for (int i = 0; i < 10; ++i) {
        auto spr = CCSprite::create("GJ_button_04.png");
        spr->setScale(0.7f);

        auto btn = CCMenuItemSpriteExtra::create(spr, target, selectCallback);
        btn->setID(fmt::format("slot-btn-{}"_spr, i));
        btn->setTag(i);
        btn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));
        btn->setCascadeOpacityEnabled(true);
        if (inPopup)
            btn->setEnabled(false);

        auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
        trashSpr->setScale(0.3f);
        trashSpr->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

        auto trashBtn = CCMenuItemSpriteExtra::create(trashSpr, target, removeCallback);
        trashBtn->setVisible(false);
        trashBtn->setTag(i);
        trashBtn->setID(fmt::format("slot-trash-btn-{}"_spr, i));

        hotbarMenu->addChild(trashBtn);
        hotbarMenu->addChild(btn);
    }

    hotbarMenu->setID("hotbar-menu"_spr);
    hotbarMenu->updateLayout();
    hotbar->addChild(hotbarMenu);

    if (!inPopup) {
        auto hotbarBtnMenu = CCMenu::create();
        hotbarBtnMenu->setPosition({0, 0});

        auto leftArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        leftArrowSpr->setScale(0.5);
        auto leftArrowBtn = CCMenuItemSpriteExtra::create(leftArrowSpr, this, menu_selector(ModEditorUI::onPageLeft));
        leftArrowBtn->setPosition({hotbar->getContentWidth() / 2 - 135, hotbar->getContentHeight() / 2});
        leftArrowBtn->setID("left-arrow"_spr);
        leftArrowBtn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

        auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
        rightArrowSpr->setFlipX(true);
        rightArrowSpr->setScale(0.5);
        auto rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(ModEditorUI::onPageRight));
        rightArrowBtn->setPosition({hotbar->getContentWidth() / 2 + 135, hotbar->getContentHeight() / 2});
        rightArrowBtn->setID("right-arrow"_spr);
        rightArrowBtn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

        auto hotbarMenuSpr = CCSprite::createWithSpriteFrameName("GJ_menuBtn_001.png");
        hotbarMenuSpr->setScale(0.5);
        auto hotbarMenuBtn = CCMenuItemSpriteExtra::create(hotbarMenuSpr, this, menu_selector(ModEditorUI::onHotbarMenu));
        hotbarMenuBtn->setPosition({hotbar->getContentWidth() / 2 + 160, hotbar->getContentHeight() / 2});
        hotbarMenuBtn->setID("hotbar-menu-btn"_spr);
        hotbarMenuBtn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

        hotbarBtnMenu->addChild(leftArrowBtn);
        hotbarBtnMenu->addChild(rightArrowBtn);
        hotbarBtnMenu->addChild(hotbarMenuBtn);

        hotbar->addChild(hotbarBtnMenu);
    }

    return hotbar;
}

bool ModEditorUI::init(LevelEditorLayer *editorLayer) {
    if (!EditorUI::init(editorLayer))
        return false;

    loadHotbar();

    for (int i = 0; i < 10; ++i) {
        std::string bindID = fmt::format("hotbar-slot-{}", i + 1);

        this->addEventListener(KeybindSettingPressedEventV3(Mod::get(), bindID),
                               [this, i](Keybind const &keybind, bool down, bool repeat, double timestamp) {
                                   if (down && !repeat) {
                                       if (auto slot = getSlotObject(i))
                                           m_selectedObjectIndex = slot;
                                       updateHotbar();

                                       return geode::ListenerResult::Stop;
                                   }
                                   return geode::ListenerResult::Propagate;
                               });
    }

    auto hotbar = createHotbarLayout(false);

    hotbar->setID("hotbar"_spr);
    this->addChild(hotbar);

    if (this->m_uiItems) {
        this->m_uiItems->addObject(hotbar);
    }

    updateHotbar();

    this->addEventListener(
        tinker::api::ui_scaling::UIScaleUpdated(), [this](float scale, bool scaleToolbars) { this->updateHotbar(); }, Priority::Last);

    return true;
}

void ModEditorUI::onHotbarMenu(CCObject *sender) { HotbarMenuPopup::create()->show(); }

void ModEditorUI::onPageLeft(CCObject *sender) {
    auto fields = m_fields.self();
    if (fields->m_hotbarPages.empty())
        return;

    if (fields->m_currentPage == 0) {
        fields->m_currentPage = fields->m_hotbarPages.size() - 1;
    } else {
        fields->m_currentPage--;
    }
    updateHotbar();
}

void ModEditorUI::onPageRight(CCObject *sender) {
    auto fields = m_fields.self();
    if (fields->m_hotbarPages.empty())
        return;

    fields->m_currentPage++;
    if (fields->m_currentPage >= fields->m_hotbarPages.size()) {
        fields->m_currentPage = 0;
    }
    updateHotbar();
}

void ModEditorUI::addPage() {
    auto fields = m_fields.self();
    fields->m_hotbarPages.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    fields->m_currentPage = fields->m_hotbarPages.size() - 1;
    saveHotbar();
    updateHotbar();
}

void ModEditorUI::removePage(int index) {
    auto fields = m_fields.self();
    if (fields->m_hotbarPages.size() <= 1) {
        auto notif = Notification::create("Cannot delete the last page!");
        notif->show();
        return;
    }

    if (index >= 0 && static_cast<size_t>(index) < fields->m_hotbarPages.size()) {
        fields->m_hotbarPages.erase(fields->m_hotbarPages.begin() + index);

        if (fields->m_currentPage >= static_cast<int>(fields->m_hotbarPages.size())) {
            fields->m_currentPage = static_cast<int>(fields->m_hotbarPages.size()) - 1;
        }

        saveHotbar();
        updateHotbar();
    }
}

int ModEditorUI::getSlotObject(int slot) {
    auto fields = m_fields.self();
    if (fields->m_currentPage >= 0 && static_cast<size_t>(fields->m_currentPage) < fields->m_hotbarPages.size() && slot >= 0 && slot < 10) {
        return fields->m_hotbarPages[fields->m_currentPage][slot];
    }
    return 0;
}

void ModEditorUI::setSlotObject(int slot, int value, int page) {
    auto fields = m_fields.self();
    int targetPage = (page == -1) ? fields->m_currentPage : page;

    if (targetPage >= 0 && static_cast<size_t>(targetPage) < fields->m_hotbarPages.size() && slot >= 0 && slot < 10) {
        fields->m_hotbarPages[targetPage][slot] = value;
    }
}

void ModEditorUI::loadHotbar() {
    auto fields = m_fields.self();
    fields->m_hotbarPages.clear();

    if (auto filePathRes = (Mod::get()->getSaveDir() / "hotbar.txt").toStdPath()) {
        std::ifstream file(filePathRes.unwrap());
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                if (line.empty())
                    continue;

                std::istringstream iss(line);
                std::array<int, 10> pageData{0};
                for (int i = 0; i < 10; ++i) {
                    if (!(iss >> pageData[i])) {
                        break;
                    }
                }
                fields->m_hotbarPages.push_back(pageData);
            }
            file.close();
        }
    }

    if (fields->m_hotbarPages.empty()) {
        fields->m_hotbarPages.push_back({1, 8, 1734, 31, 0, 0, 0, 0, 0, 0});
    }

    if (fields->m_currentPage >= fields->m_hotbarPages.size()) {
        fields->m_currentPage = 0;
    }
}

void ModEditorUI::saveHotbar() {
    auto filePathRes = (Mod::get()->getSaveDir() / "hotbar.txt").toStdPath();
    if (!filePathRes)
        return;

    std::ofstream file(filePathRes.unwrap());
    auto fields = m_fields.self();

    if (file.is_open()) {
        for (const auto &page : fields->m_hotbarPages) {
            for (int i = 0; i < 10; ++i) {
                file << page[i] << (i == 9 ? "" : " ");
            }
            file << "\n";
        }
        file.close();
    }
}

void ModEditorUI::applySelectionTint(CCNode *node) {
    if (!node)
        return;

    if (auto bgSlice = node->getChildByType<cocos2d::extension::CCScale9Sprite>(0)) {
        bgSlice->setColor(cocos2d::ccColor3B{127, 127, 127});
    } else if (typeinfo_cast<ButtonSprite *>(node) && node->getChildrenCount() > 1) {
        if (auto sprite = typeinfo_cast<CCSprite *>(node->getChildByIndex(1)))
            sprite->setColor(cocos2d::ccColor3B{127, 127, 127});
    }
}

void ModEditorUI::updateHotbar(bool triggeredByCreateMenuSelect) {
    auto hotbar = this->getChildByID("hotbar"_spr);
    if (!hotbar)
        return;

    if (auto tabsMenu = this->getChildByID("build-tabs-menu")) {
        CCPoint tabsPos = tabsMenu->getPosition();
        hotbar->setPosition({tabsPos.x, tabsPos.y + 37.5f + Mod::get()->getSettingValue<int64_t>("hotbar-y-offset")});
    }

    auto menu = hotbar->getChildByID("hotbar-menu"_spr);
    if (!menu)
        return;

    bool isBuildMode = (this->m_selectedMode == 2);

    hotbar->setVisible(isBuildMode && m_fields->m_uiVisible);

    if (!isBuildMode || !m_fields->m_uiVisible)
        return;

    for (int i = 0; i < 10; ++i) {
        auto slotBtn = static_cast<CCMenuItemSpriteExtra *>(menu->getChildByID(fmt::format("slot-btn-{}"_spr, i)));
        auto trashBtn = static_cast<CCMenuItemSpriteExtra *>(menu->getChildByID(fmt::format("slot-trash-btn-{}"_spr, i)));

        if (!slotBtn || !trashBtn)
            continue;

        updateHotbarSlot(i, slotBtn, trashBtn);
    }

    if (!triggeredByCreateMenuSelect)
        EditorUI::updateCreateMenu(false);
}

void ModEditorUI::updateHotbarSlot(int slot, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn) {
    int objectID = getSlotObject(slot);

    if (objectID != 0) {
        updateFilledSlot(slot, objectID, slotBtn, trashBtn);
    } else {
        updateEmptySlot(slot, slotBtn, trashBtn);
    }
}

void ModEditorUI::updateFilledSlot(int slot, int objectID, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn) {
    CreateMenuItem *createBtn = nullptr;

    if (objectID > 0) {
        createBtn = this->getCreateBtn(objectID, 4);

        if (!createBtn) {
            createBtn = this->getCreateBtn(1, 4);

            setSlotObject(slot, 1);
            saveHotbar();

            auto notif = Notification::create(fmt::format("Object in slot {} failed to load", slot + 1));
            notif->show();
        }
    } else if (objectID < 0) {
        auto objStr = GameManager::get()->stringForCustomObject(objectID);

        if (!objStr.empty()) {
            createBtn = this->menuItemFromObjectString(objStr, objectID);
        } else {
            setSlotObject(slot, 0);
            saveHotbar();
        }
    }

    CCNode *spr = nullptr;

    if (createBtn)
        spr = createBtn->getNormalImage();

    if (!spr)
        spr = CCSprite::create("GJ_button_04.png");

    if (!spr)
        return;

    spr->setScale(0.7f);

    if (auto rgba = typeinfo_cast<cocos2d::CCRGBAProtocol *>(spr)) {
        rgba->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));
        rgba->setCascadeOpacityEnabled(true);
    }

    if (this->m_selectedObjectIndex == objectID) {
        applySelectionTint(spr);

        trashBtn->setPosition(slotBtn->getPosition() + CCPoint{0, 17.5f});

        trashBtn->setVisible(true);
    } else {
        trashBtn->setVisible(false);
    }

    slotBtn->setSprite(static_cast<CCSprite *>(spr));
    slotBtn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));
    slotBtn->setCascadeOpacityEnabled(true);
}

void ModEditorUI::updateEmptySlot(int slot, CCMenuItemSpriteExtra *slotBtn, CCMenuItemSpriteExtra *trashBtn) {
    CCSprite *spr = nullptr;

    if (m_fields->m_currentPage == m_fields->m_assigningPage && slot == m_fields->m_assigningSlot) {
        spr = CCSprite::createWithSpriteFrameName("GJ_cancelDownloadBtn_001.png");
        spr->setScale(0.5f);
    } else {
        spr = CCSprite::createWithSpriteFrameName("GJ_plus3Btn_001.png");
        spr->setScale(0.7f);
    }

    slotBtn->setSprite(spr);
    slotBtn->setOpacity(Mod::get()->getSettingValue<int64_t>("hotbar-opacity"));

    trashBtn->setVisible(false);
}

void ModEditorUI::onCreateButton(CCObject *sender) {
    EditorUI::onCreateButton(sender);

    if (auto item = typeinfo_cast<CreateMenuItem *>(sender)) {
        auto fields = m_fields.self();
        if (this->m_selectedObjectIndex != 0 && fields->m_assigningSlot > -1) {
            setSlotObject(fields->m_assigningSlot, item->m_objectID, fields->m_assigningPage);
            fields->m_assigningSlot = -1;
            fields->m_assigningPage = -1;
            saveHotbar();
        }
    }
    updateHotbar();
}

void ModEditorUI::onObjectSelected(CCObject *sender) {
    if (!sender)
        return;
    int tag = sender->getTag();
    auto fields = m_fields.self();
    int objectID = getSlotObject(tag);

    if (objectID == 0) {
        if (fields->m_assigningPage == fields->m_currentPage && tag == fields->m_assigningSlot) {
            fields->m_assigningPage = -1;
            fields->m_assigningSlot = -1;
        } else {
            this->m_selectedObjectIndex = 0;
            fields->m_assigningSlot = tag;
            fields->m_assigningPage = fields->m_currentPage;
            auto notif = Notification::create(fmt::format("Select an object to add to slot {}", tag + 1));
            notif->show();
        }
    } else {
        if (m_selectedObjectIndex != objectID) {
            m_selectedObjectIndex = objectID;
        } else {
            m_selectedObjectIndex = 0;
        }
    }
    updateHotbar();
}

void ModEditorUI::onObjectRemoved(CCObject *sender) {
    if (!sender)
        return;
    int tag = sender->getTag();
    setSlotObject(tag, 0);
    m_selectedObjectIndex = 0;
    saveHotbar();
    updateHotbar();
}

void ModEditorUI::toggleMode(CCObject *sender) {
    EditorUI::toggleMode(sender);
    updateHotbar(true);
}

void ModEditorUI::showUI(bool show) {
    EditorUI::showUI(show);
    m_fields->m_uiVisible = show;
    updateHotbar();
}

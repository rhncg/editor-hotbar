#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <fstream>

using namespace geode::prelude;

#include <Geode/modify/GameManager.hpp>
class $modify(GameManager) {
    gd::string stringForCustomObject(int customObjectID) {
        gd::string ret = GameManager::stringForCustomObject(customObjectID);
        log::info("customObjectID: {}", customObjectID);
        return ret;
    }
};

#include <Geode/modify/EditorUI.hpp>
class $modify(ModEditorUI, EditorUI) {
    struct Fields {
        std::array<int, 10> m_slotObjects{1, 8, 1734, 31, 0, 0, 0, 0, 0, 0};
        int m_assigningSlot = -1;
    };

    bool init(LevelEditorLayer *editorLayer) {
        if (!EditorUI::init(editorLayer))
            return false;

        loadHotbar();

        for (int i = 0; i < 10; ++i) {
            std::string bindID = fmt::format("hotbar-slot-{}", i + 1);

            this->addEventListener(
                KeybindSettingPressedEventV3(Mod::get(), bindID),
                [this, i](Keybind const &keybind, bool down, bool repeat,
                          double timestamp) {
                    if (down && !repeat) {
                        if (auto slot = this->m_fields->m_slotObjects[i])
                            m_selectedObjectIndex = slot;
                        updateHotbar();
                        return geode::ListenerResult::Stop;
                    }
                    return geode::ListenerResult::Propagate;
                });
        }

        auto slice = geode::NineSlice::create("square02b_001.png");
        slice->setContentSize({245, 30});

        auto tabsMenu = this->getChildByID("build-tabs-menu");
        if (tabsMenu) {
            CCPoint tabsPos = tabsMenu->getPosition();
            slice->setPosition({tabsPos.x, tabsPos.y - 40.0f});
        } else {
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            slice->setPosition(winSize.width / 2, winSize.height / 2 - 45);
        }

        slice->setColor(
            Mod::get()->getSettingValue<cocos2d::ccColor3B>("hotbar-color"));

        auto hotbarMenu = CCMenu::create();
        hotbarMenu->setContentSize(slice->getContentSize());
        hotbarMenu->setPosition(slice->getContentSize() / 2.0f);

        auto layout = AxisLayout::create(Axis::Row)
                          ->setGap(5.0f)
                          ->setGrowCrossAxis(true)
                          ->setCrossAxisOverflow(false)
                          ->setAxisAlignment(AxisAlignment::Center)
                          ->setCrossAxisAlignment(AxisAlignment::Center)
                          ->setAutoScale(true)
                          ->setPadding({5, 0, 5, 0});

        hotbarMenu->setLayout(layout);

        for (int i = 0; i < 10; ++i) {
            auto spr = CCSprite::create("GJ_button_04.png");
            spr->setScale(0.7f);
            auto btn = CCMenuItemSpriteExtra::create(
                spr, this, menu_selector(ModEditorUI::onObjectSelected));
            btn->setID(fmt::format("slot-btn-{}"_spr, i));
            btn->setTag(i);

            auto trashSpr =
                CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
            trashSpr->setScale(0.3f);
            auto trashBtn = CCMenuItemSpriteExtra::create(
                trashSpr, this, menu_selector(ModEditorUI::onObjectRemoved));
            trashBtn->setVisible(false);
            trashBtn->setTag(i);
            trashBtn->setID(fmt::format("slot-trash-btn-{}"_spr, i));

            hotbarMenu->addChild(trashBtn);
            hotbarMenu->addChild(btn);
        }

        slice->setID("hotbar"_spr);
        hotbarMenu->setID("hotbar-menu"_spr);
        hotbarMenu->updateLayout();
        slice->addChild(hotbarMenu);
        this->addChild(slice);

        if (this->m_uiItems) {
            this->m_uiItems->addObject(slice);
        }

        updateHotbar();

        return true;
    }

    void loadHotbar() {
        auto filePath = Mod::get()->getSaveDir() / "hotbar.txt";
        std::ifstream file(filePath);
        auto fields = m_fields.self();

        if (file.is_open()) {
            for (int i = 0; i < 10; ++i) {
                if (!(file >> fields->m_slotObjects[i])) {
                    break;
                }
            }
            file.close();
        } else {
            fields->m_slotObjects = {1, 8, 1734, 31, 0, 0, 0, 0, 0, 0};
        }
    }

    void saveHotbar() {
        auto filePath = Mod::get()->getSaveDir() / "hotbar.txt";
        std::ofstream file(filePath);
        auto fields = m_fields.self();

        if (file.is_open()) {
            for (int i = 0; i < 10; ++i) {
                file << numToString(fields->m_slotObjects[i]) << " ";
            }
            file.close();
        }
    }

    // TODO: add setting to choose whether or not to tint gameobject
    void applySelectionTint(CCNode *node) {
        if (!node)
            return;

        if (auto bgSlice =
                node->getChildByType<cocos2d::extension::CCScale9Sprite>(0)) {
            bgSlice->setColor(cocos2d::ccColor3B{127, 127, 127});
        } else if (typeinfo_cast<ButtonSprite *>(node) &&
                   node->getChildrenCount() > 1) {
            if (auto sprite =
                    typeinfo_cast<CCSprite *>(node->getChildByIndex(1)))
                sprite->setColor(cocos2d::ccColor3B{127, 127, 127});
        }
    }

    void updateHotbar() {
        auto hotbar = this->getChildByID("hotbar"_spr);
        if (!hotbar)
            return;

        if (auto tabsMenu = this->getChildByID("build-tabs-menu")) {
            CCPoint tabsPos = tabsMenu->getPosition();
            hotbar->setPosition({tabsPos.x, tabsPos.y + 37.5f});
        }

        auto menu = hotbar->getChildByID("hotbar-menu"_spr);
        if (!menu)
            return;

        bool isBuildMode = (this->m_selectedMode == 2);
        hotbar->setVisible(isBuildMode);
        if (!isBuildMode)
            return;

        auto fields = m_fields.self();

        for (int i = 0; i < 10; ++i) {
            auto slotBtn = static_cast<CCMenuItemSpriteExtra *>(
                menu->getChildByID(fmt::format("slot-btn-{}"_spr, i)));
            auto trashBtn = static_cast<CCMenuItemSpriteExtra *>(
                menu->getChildByID(fmt::format("slot-trash-btn-{}"_spr, i)));

            if (!slotBtn || !trashBtn)
                continue;

            int objectID = fields->m_slotObjects[i];

            if (objectID != 0) {
                CreateMenuItem *createBtn = nullptr;

                if (objectID > 0) {
                    createBtn = this->getCreateBtn(objectID, 4);
                    if (!createBtn) {
                        createBtn = this->getCreateBtn(1, 4);
                        fields->m_slotObjects[i] = 1;
                        saveHotbar();
                        auto notif = Notification::create(fmt::format(
                            "Object in slot {} failed to load", i + 1));
                        notif->show();
                    }
                } else if (objectID < 0) {
                    createBtn = this->menuItemFromObjectString(
                        GameManager::get()->stringForCustomObject(objectID),
                        objectID);
                }

                CCNode *spr = nullptr;
                if (createBtn) {
                    spr = createBtn->getNormalImage();
                }

                if (!spr) {
                    spr = CCSprite::create("GJ_button_04.png");
                }

                if (spr) {
                    spr->setScale(0.7f);

                    if (this->m_selectedObjectIndex == objectID) {
                        applySelectionTint(spr);

                        trashBtn->setPosition(slotBtn->getPosition() +
                                              CCPoint{0, 17.5f});
                        trashBtn->setVisible(true);
                    } else {
                        trashBtn->setVisible(false);
                    }

                    slotBtn->setSprite(static_cast<CCSprite *>(spr));
                }
            } else {
                auto spr =
                    CCSprite::createWithSpriteFrameName("GJ_plus3Btn_001.png");
                spr->setScale(0.7f);
                slotBtn->setSprite(spr);
                trashBtn->setVisible(false);
            }
        }
    }

    void onCreateButton(CCObject *sender) {
        EditorUI::onCreateButton(sender);

        if (auto item = typeinfo_cast<CreateMenuItem *>(sender)) {
            auto fields = m_fields.self();
            if (this->m_selectedObjectIndex != 0 &&
                fields->m_assigningSlot > -1) {
                fields->m_slotObjects[fields->m_assigningSlot] =
                    item->m_objectID;
                fields->m_assigningSlot = -1;
                saveHotbar();
            }
        }
        updateHotbar();
    }

    void onObjectSelected(CCObject *sender) {
        if (!sender)
            return;
        int tag = sender->getTag();
        auto fields = m_fields.self();
        if (fields->m_slotObjects[tag] == 0) {
            this->m_selectedObjectIndex = 0;
            fields->m_assigningSlot = tag;
            auto notif = Notification::create(
                fmt::format("Select an object to add to slot {}", tag + 1));
            notif->show();
        } else {
            if (m_selectedObjectIndex != fields->m_slotObjects[tag]) {
                m_selectedObjectIndex = fields->m_slotObjects[tag];
            } else {
                m_selectedObjectIndex = 0;
            }
        }
        EditorUI::updateCreateMenu(false);
        updateHotbar();
    }

    void onObjectRemoved(CCObject *sender) {
        if (!sender)
            return;
        int tag = sender->getTag();
        this->m_fields->m_slotObjects[tag] = 0;
        m_selectedObjectIndex = 0;
        saveHotbar();
        updateHotbar();
    }

    void updateCreateMenu(bool selectTab) {
        EditorUI::updateCreateMenu(selectTab);
        updateHotbar();
    }

    void showUI(bool show) {
        EditorUI::showUI(show);
        updateHotbar();
    }
};
#include <Geode/Geode.hpp>
#include <Geode/ui/NineSlice.hpp>
#include <Geode/ui/ScrollLayer.hpp>
#include <fstream>
#include <sstream>

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
        std::vector<std::array<int, 10>> m_hotbarPages;
        int m_currentPage = 0;
        int m_assigningPage = -1;
        int m_assigningSlot = -1;
    };

    geode::NineSlice *createHotbarLayout(bool inPopup = false) {
        auto hotbar = geode::NineSlice::create("square02b_001.png");
        hotbar->setContentSize({245, 30});

        if (Mod::get()->getSettingValue<bool>("show-hotbar-outline")) {
            auto outline = geode::NineSlice::create("GJ_square07.png");
            outline->setContentSize({245, 30});
            outline->setPosition({245 / 2.0f, 30 / 2.0f});
            hotbar->addChild(outline);
        }

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

        // prolly a better way to do this
        CCObject *target = inPopup ? nullptr : this;
        auto selectCallback = inPopup ? nullptr : menu_selector(ModEditorUI::onObjectSelected);
        auto removeCallback = inPopup ? nullptr : menu_selector(ModEditorUI::onObjectRemoved);

        for (int i = 0; i < 10; ++i) {
            auto spr = CCSprite::create("GJ_button_04.png");
            spr->setScale(0.7f);
            auto btn = CCMenuItemSpriteExtra::create(spr, target, selectCallback);
            btn->setID(fmt::format("slot-btn-{}"_spr, i));
            btn->setTag(i);

            auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
            trashSpr->setScale(0.3f);
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

            auto rightArrowSpr = CCSprite::createWithSpriteFrameName("GJ_arrow_03_001.png");
            rightArrowSpr->setFlipX(true);
            rightArrowSpr->setScale(0.5);
            auto rightArrowBtn = CCMenuItemSpriteExtra::create(rightArrowSpr, this, menu_selector(ModEditorUI::onPageRight));
            rightArrowBtn->setPosition({hotbar->getContentWidth() / 2 + 135, hotbar->getContentHeight() / 2});
            rightArrowBtn->setID("right-arrow"_spr);

            auto hotbarMenuSpr = CCSprite::createWithSpriteFrameName("GJ_menuBtn_001.png");
            hotbarMenuSpr->setScale(0.5);
            auto hotbarMenuBtn = CCMenuItemSpriteExtra::create(hotbarMenuSpr, this, menu_selector(ModEditorUI::onHotbarMenu));
            hotbarMenuBtn->setPosition({hotbar->getContentWidth() / 2 + 160, hotbar->getContentHeight() / 2});
            hotbarMenuBtn->setID("hotbar-menu-btn"_spr);

            hotbarBtnMenu->addChild(leftArrowBtn);
            hotbarBtnMenu->addChild(rightArrowBtn);
            hotbarBtnMenu->addChild(hotbarMenuBtn);

            hotbar->addChild(hotbarBtnMenu);
        }

        return hotbar;
    }

    bool init(LevelEditorLayer *editorLayer) {
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

        auto tabsMenu = this->getChildByID("build-tabs-menu");
        if (tabsMenu) {
            CCPoint tabsPos = tabsMenu->getPosition();
            hotbar->setPosition({tabsPos.x, tabsPos.y - 45.0f});
        } else {
            auto winSize = CCDirector::sharedDirector()->getWinSize();
            hotbar->setPosition(winSize.width / 2, winSize.height / 2 - 45);
        }

        hotbar->setID("hotbar"_spr);
        this->addChild(hotbar);

        if (this->m_uiItems) {
            this->m_uiItems->addObject(hotbar);
        }

        updateHotbar();

        return true;
    }

    void onHotbarMenu(CCObject *sender);

    void onPageLeft(CCObject *sender) {
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

    void onPageRight(CCObject *sender) {
        auto fields = m_fields.self();
        if (fields->m_hotbarPages.empty())
            return;

        fields->m_currentPage++;
        if (fields->m_currentPage >= fields->m_hotbarPages.size()) {
            fields->m_currentPage = 0;
        }
        updateHotbar();
    }

    void addPage() {
        auto fields = m_fields.self();
        fields->m_hotbarPages.push_back({0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        fields->m_currentPage = fields->m_hotbarPages.size() - 1;
        saveHotbar();
        updateHotbar();
    }

    void removePage(int index) {
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

    int getSlotObject(int slot) {
        auto fields = m_fields.self();
        if (fields->m_currentPage >= 0 && static_cast<size_t>(fields->m_currentPage) < fields->m_hotbarPages.size() && slot >= 0 && slot < 10) {
            return fields->m_hotbarPages[fields->m_currentPage][slot];
        }
        return 0;
    }

    void setSlotObject(int slot, int value, int page = -1) {
        auto fields = m_fields.self();
        int targetPage = (page == -1) ? fields->m_currentPage : page;

        if (targetPage >= 0 && static_cast<size_t>(targetPage) < fields->m_hotbarPages.size() && slot >= 0 && slot < 10) {
            fields->m_hotbarPages[targetPage][slot] = value;
        }
    }

    void loadHotbar() {
        auto filePath = Mod::get()->getSaveDir() / "hotbar.txt";
        std::ifstream file(filePath);
        auto fields = m_fields.self();

        fields->m_hotbarPages.clear();

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

        if (fields->m_hotbarPages.empty()) {
            fields->m_hotbarPages.push_back({1, 8, 1734, 31, 0, 0, 0, 0, 0, 0});
        }

        if (fields->m_currentPage >= fields->m_hotbarPages.size()) {
            fields->m_currentPage = 0;
        }
    }

    void saveHotbar() {
        auto filePath = Mod::get()->getSaveDir() / "hotbar.txt";
        std::ofstream file(filePath);
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

    void applySelectionTint(CCNode *node) {
        if (!node)
            return;

        if (auto bgSlice = node->getChildByType<cocos2d::extension::CCScale9Sprite>(0)) {
            bgSlice->setColor(cocos2d::ccColor3B{127, 127, 127});
        } else if (typeinfo_cast<ButtonSprite *>(node) && node->getChildrenCount() > 1) {
            if (auto sprite = typeinfo_cast<CCSprite *>(node->getChildByIndex(1)))
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

        for (int i = 0; i < 10; ++i) {
            auto slotBtn = static_cast<CCMenuItemSpriteExtra *>(menu->getChildByID(fmt::format("slot-btn-{}"_spr, i)));
            auto trashBtn = static_cast<CCMenuItemSpriteExtra *>(menu->getChildByID(fmt::format("slot-trash-btn-{}"_spr, i)));

            if (!slotBtn || !trashBtn)
                continue;

            int objectID = getSlotObject(i);

            if (objectID != 0) {
                CreateMenuItem *createBtn = nullptr;

                if (objectID > 0) {
                    createBtn = this->getCreateBtn(objectID, 4);
                    if (!createBtn) {
                        createBtn = this->getCreateBtn(1, 4);
                        setSlotObject(i, 1);
                        saveHotbar();
                        auto notif = Notification::create(fmt::format("Object in slot {} failed to load", i + 1));
                        notif->show();
                    }
                } else if (objectID < 0) {
                    createBtn = this->menuItemFromObjectString(GameManager::get()->stringForCustomObject(objectID), objectID);
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

                        trashBtn->setPosition(slotBtn->getPosition() + CCPoint{0, 17.5f});
                        trashBtn->setVisible(true);
                    } else {
                        trashBtn->setVisible(false);
                    }

                    slotBtn->setSprite(static_cast<CCSprite *>(spr));
                }
            } else {
                auto spr = CCSprite::createWithSpriteFrameName("GJ_plus3Btn_001.png");
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
            if (this->m_selectedObjectIndex != 0 && fields->m_assigningSlot > -1) {
                setSlotObject(fields->m_assigningSlot, item->m_objectID, fields->m_assigningPage);
                fields->m_assigningSlot = -1;
                fields->m_assigningPage = -1;
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
        int objectID = getSlotObject(tag);

        if (objectID == 0) {
            this->m_selectedObjectIndex = 0;
            fields->m_assigningSlot = tag;
            fields->m_assigningPage = fields->m_currentPage;
            auto notif = Notification::create(fmt::format("Select an object to add to slot {}", tag + 1));
            notif->show();
        } else {
            if (m_selectedObjectIndex != objectID) {
                m_selectedObjectIndex = objectID;
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
        setSlotObject(tag, 0);
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

class HotbarMenuPopup : public geode::Popup {
  protected:
    ScrollLayer *m_scrollLayer = nullptr;

    bool init() {
        if (!Popup::init(310.f, 210.f))
            return false;

        this->setTitle("Hotbar Settings");
        setupList();

        return true;
    }

    void setupList() {
        auto editorUI = EditorUI::get();
        if (!editorUI)
            return;

        auto modEditor = static_cast<ModEditorUI *>(editorUI);
        auto fields = modEditor->m_fields.self();

        CCSize scrollLayerSize = {290.f, 150.f};

        float previousOffsetY = 0.0f;
        bool isFirstLoad = (m_scrollLayer == nullptr);

        if (!isFirstLoad) {
            previousOffsetY = m_scrollLayer->m_contentLayer->getPositionY();
            m_scrollLayer->m_contentLayer->removeAllChildren();
        } else {
            m_scrollLayer = ScrollLayer::create(scrollLayerSize);
            m_scrollLayer->setPosition({(m_mainLayer->getContentWidth() - scrollLayerSize.width) / 2.0f, 15.f});
            this->m_mainLayer->addChild(m_scrollLayer);
        }

        float itemHeight = 38.0f;
        float rawContentHeight = (fields->m_hotbarPages.size() + 1) * itemHeight;
        float totalHeight = std::max(rawContentHeight, scrollLayerSize.height);

        auto contentLayer = m_scrollLayer->m_contentLayer;
        contentLayer->setContentSize({scrollLayerSize.width, totalHeight});

        auto layout = AxisLayout::create(Axis::Column)
                          ->setGap(8.0f)
                          ->setAxisAlignment(AxisAlignment::End)
                          ->setCrossAxisAlignment(AxisAlignment::Center)
                          ->setAxisReverse(true)
                          ->setAutoScale(false);

        contentLayer->setLayout(layout);

        for (size_t pageIdx = 0; pageIdx < fields->m_hotbarPages.size(); ++pageIdx) {
            // theres got to be a better way to do this
            auto container = CCNode::create();
            container->setContentSize({280.f, 30.f});

            auto hotbar = modEditor->createHotbarLayout(true);
            hotbar->setPosition({127.5f, 15.f});

            if (auto hotbarMenu = hotbar->getChildByID("hotbar-menu"_spr)) {
                for (int slot = 0; slot < 10; ++slot) {
                    auto slotBtn = static_cast<CCMenuItemSpriteExtra *>(hotbarMenu->getChildByID(fmt::format("slot-btn-{}"_spr, slot)));
                    if (!slotBtn)
                        continue;

                    int objectID = fields->m_hotbarPages[pageIdx][slot];

                    if (objectID != 0) {
                        CreateMenuItem *createBtn = nullptr;
                        if (objectID > 0) {
                            createBtn = modEditor->getCreateBtn(objectID, 4);
                        } else if (objectID < 0) {
                            createBtn = modEditor->menuItemFromObjectString(GameManager::get()->stringForCustomObject(objectID), objectID);
                        }

                        CCNode *spr = nullptr;
                        if (createBtn)
                            spr = createBtn->getNormalImage();
                        if (!spr)
                            spr = CCSprite::create("GJ_button_04.png");

                        if (spr) {
                            spr->setScale(0.7f);
                            slotBtn->setSprite(static_cast<CCSprite *>(spr));
                        }
                    } else {
                        auto spr = CCSprite::createWithSpriteFrameName("GJ_plus3Btn_001.png");
                        spr->setScale(0.7f);
                        slotBtn->setSprite(spr);
                    }
                }
            }

            container->addChild(hotbar);

            auto sideMenu = CCMenu::create();
            sideMenu->setPosition({0, 0});

            auto trashSpr = CCSprite::createWithSpriteFrameName("GJ_trashBtn_001.png");
            trashSpr->setScale(0.6f);

            auto trashBtn = CCMenuItemSpriteExtra::create(trashSpr, this, menu_selector(HotbarMenuPopup::onDeletePage));
            trashBtn->setTag(static_cast<int>(pageIdx));
            trashBtn->setPosition({265.f, 15.f});

            sideMenu->addChild(trashBtn);
            container->addChild(sideMenu);

            contentLayer->addChild(container);
        }

        auto addPageMenu = CCMenu::create();
        addPageMenu->setContentSize({280.f, 30.f});

        auto addPageSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
        addPageSpr->setScale(0.8f);

        auto addPageBtn = CCMenuItemSpriteExtra::create(addPageSpr, this, menu_selector(HotbarMenuPopup::onAddPage));
        addPageBtn->setPosition(addPageMenu->getContentSize() / 2.0f);

        addPageMenu->addChild(addPageBtn);
        contentLayer->addChild(addPageMenu);

        contentLayer->updateLayout();

        if (isFirstLoad) {
            m_scrollLayer->scrollToTop();
        } else {
            float minY = scrollLayerSize.height - totalHeight;
            float clampedY = std::min(0.0f, std::max(minY, previousOffsetY));
            contentLayer->setPositionY(clampedY);
        }
    }

    void onDeletePage(CCObject *sender) {
        if (!sender)
            return;
        size_t pageIdx = static_cast<size_t>(sender->getTag());

        if (auto modEditor = static_cast<ModEditorUI *>(EditorUI::get())) {
            modEditor->removePage(pageIdx);
            setupList();
        }
    }

    void onAddPage(CCObject *sender) {
        if (auto modEditor = static_cast<ModEditorUI *>(EditorUI::get())) {
            modEditor->addPage();
            setupList();
        }
    }

  public:
    static HotbarMenuPopup *create() {
        auto ret = new HotbarMenuPopup();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        CC_SAFE_DELETE(ret);
        return nullptr;
    }
};

void ModEditorUI::onHotbarMenu(CCObject *sender) { HotbarMenuPopup::create()->show(); }
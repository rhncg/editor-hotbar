#include "ui/HotbarMenuPopup.hpp"
#include <asp/fs/std.hpp>

bool HotbarMenuPopup::init() {
    if (!Popup::init(310.f, 210.f))
        return false;

    this->setTitle("Hotbar Settings");
    setupList();

    return true;
}

void HotbarMenuPopup::setupList() {
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
        m_scrollLayer->ignoreAnchorPointForPosition(false);
        m_scrollLayer->setAnchorPoint({0.5f, 0.5f});
        m_scrollLayer->setPosition({m_mainLayer->getContentWidth() / 2.0f, m_mainLayer->getContentHeight() / 2.0f - 10.f});
        this->m_mainLayer->addChild(m_scrollLayer);
    }

    float contentHeight = (fields->m_hotbarPages.size() + 1) * 38.0f; // 38 item height
    float totalHeight = std::max(contentHeight, scrollLayerSize.height);

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
                        auto objStr = GameManager::get()->stringForCustomObject(objectID);
                        if (!objStr.empty())
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

    auto popupOptionsMenu = CCMenu::create();
    popupOptionsMenu->setContentSize({280.f, 30.f});

    auto popupOptionsLayout = AxisLayout::create(Axis::Row)
                                  ->setGap(5.0f)
                                  ->setGrowCrossAxis(true)
                                  ->setCrossAxisOverflow(false)
                                  ->setAxisAlignment(AxisAlignment::Center)
                                  ->setCrossAxisAlignment(AxisAlignment::Center)
                                  ->setAutoScale(true)
                                  ->setPadding({5, 0, 5, 0});

    popupOptionsMenu->setLayout(popupOptionsLayout);

    auto addPageSpr = CCSprite::createWithSpriteFrameName("GJ_plusBtn_001.png");
    addPageSpr->setScale(0.7f);

    auto addPageBtn = CCMenuItemSpriteExtra::create(addPageSpr, this, menu_selector(HotbarMenuPopup::onAddPage));

    auto modSettingsSpr = CCSprite::createWithSpriteFrameName("GJ_optionsBtn_001.png");
    modSettingsSpr->setScale(0.7f);
    auto modSettingsBtn = CCMenuItemSpriteExtra::create(modSettingsSpr, this, menu_selector(HotbarMenuPopup::onOpenModSettings));

    popupOptionsMenu->addChild(addPageBtn);
    popupOptionsMenu->addChild(modSettingsBtn);
    contentLayer->addChild(popupOptionsMenu);

    popupOptionsMenu->updateLayout();
    contentLayer->updateLayout();

    if (isFirstLoad) {
        m_scrollLayer->scrollToTop();
    } else {
        float minY = scrollLayerSize.height - totalHeight;
        float clampedY = std::min(0.0f, std::max(minY, previousOffsetY));
        contentLayer->setPositionY(clampedY);
    }
}

void HotbarMenuPopup::onDeletePage(CCObject *sender) {
    if (!sender)
        return;

    size_t pageIdx = static_cast<size_t>(sender->getTag());

    geode::createQuickPopup("Delete Page", "Are you sure you want to delete this page?", "No", "Yes", [this, pageIdx](auto, bool btn2) {
        if (btn2) {
            if (auto modEditor = static_cast<ModEditorUI *>(EditorUI::get())) {
                modEditor->removePage(pageIdx);
                this->setupList();
            }
        }
    });
}

void HotbarMenuPopup::onAddPage(CCObject *sender) {
    if (auto modEditor = static_cast<ModEditorUI *>(EditorUI::get())) {
        modEditor->addPage();
        setupList();
    }
}

void HotbarMenuPopup::onOpenModSettings(CCObject *sender) {
    geode::openSettingsPopup(Mod::get(), false);
}

HotbarMenuPopup *HotbarMenuPopup::create() {
    auto ret = new HotbarMenuPopup();
    if (ret && ret->init()) {
        ret->autorelease();
        return ret;
    }
    CC_SAFE_DELETE(ret);
    return nullptr;
}
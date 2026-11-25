#include <Geode/Geode.hpp>
#include <Geode/modify/GameManager.hpp>

using namespace geode::prelude;

class $modify(GameManager) {
    std::string stringForCustomObject(int customObjectID) {
        return GameManager::stringForCustomObject(customObjectID);
    }
};
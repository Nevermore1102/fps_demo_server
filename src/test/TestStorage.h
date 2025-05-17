#pragma once

#include <string>
#include <spdlog/spdlog.h>
#include "../data/Storage.h"
#include "../data/PlayerData.h"

namespace test {

class TestStorage {
public:
    static bool runAllTests();
    
private:
    static bool testPlayerDataSaveLoad();
    static bool testPlayerStateValidation();
};

} // namespace test 
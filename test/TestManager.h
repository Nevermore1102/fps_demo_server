#pragma once
#include <memory>
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <iostream>
#include "TestBase.h"

class TestManager {
public:
    static TestManager& getInstance() {
        static TestManager instance;
        return instance;
    }

    // 注册测试用例
    void registerTest(const std::string& name, std::function<std::unique_ptr<TestBase>(const std::string&, uint16_t)> factory) {
        tests_[name] = factory;
    }

    // 获取所有测试用例名称
    std::vector<std::string> getTestNames() const {
        std::vector<std::string> names;
        for (const auto& test : tests_) {
            names.push_back(test.first);
        }
        return names;
    }

    // 运行指定的测试用例
    bool runTest(const std::string& name, const std::string& host, uint16_t port) {
        auto it = tests_.find(name);
        if (it == tests_.end()) {
            std::cerr << "Test not found: " << name << std::endl;
            return false;
        }

        auto test = it->second(host, port);
        std::cout << "Running test: " << name << std::endl;
        return test->run();
    }

private:
    TestManager() = default;
    ~TestManager() = default;
    TestManager(const TestManager&) = delete;
    TestManager& operator=(const TestManager&) = delete;

    std::map<std::string, std::function<std::unique_ptr<TestBase>(const std::string&, uint16_t)>> tests_;
}; 
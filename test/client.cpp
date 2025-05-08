#include <iostream>
#include <string>
#include "TestManager.h"
#include "ConnectionTest.h"
#include "ServerShutdownTest.h"

void printUsage(const char* program) {
    std::cout << "Usage: " << program << " [test_name]" << std::endl;
    std::cout << "Available tests:" << std::endl;
    auto& manager = TestManager::getInstance();
    for (const auto& name : manager.getTestNames()) {
        std::cout << "  " << name << std::endl;
    }
}

int main(int argc, char* argv[]) {
    // 注册测试用例
    auto& manager = TestManager::getInstance();
    manager.registerTest("connection", 
        std::function<std::unique_ptr<TestBase>(const std::string&, uint16_t)>(
            [](const std::string& host, uint16_t port) -> std::unique_ptr<TestBase> {
                return std::make_unique<ConnectionTest>(host, port);
            }
        )
    );

    manager.registerTest("shutdown", 
        std::function<std::unique_ptr<TestBase>(const std::string&, uint16_t)>(
            [](const std::string& host, uint16_t port) -> std::unique_ptr<TestBase> {
                return std::make_unique<ServerShutdownTest>(host, port);
            }
        )
    );

    // 默认服务器地址和端口
    const std::string host = "127.0.0.1";
    const uint16_t port = 8888;

    if (argc > 1) {
        // 运行指定的测试
        std::string testName = argv[1];
        if (!manager.runTest(testName, host, port)) {
            std::cerr << "Test failed: " << testName << std::endl;
            return 1;
        }
    } else {
        // 显示可用测试列表
        printUsage(argv[0]);
        return 1;
    }

    return 0;
} 
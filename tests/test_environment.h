#ifndef TEST_ENVIRONMENT_H
#define TEST_ENVIRONMENT_H

#include "vktestbinding.h"

namespace vk_testing {
class Environment : public ::testing::Environment {
public:
    Environment();

    bool parse_args(int argc, char **argv);

    virtual void SetUp();
    virtual void TearDown();

    const std::vector<Device *> &devices() { return devs_; }
    Device &default_device() { return *(devs_[default_dev_]); }
    VkPhysicalDevice gpus[16];

private:
    VkApplicationInfo app_;
    int default_dev_;
    VkInstance inst;

    std::vector<Device *> devs_;
};
}
#endif // TEST_ENVIRONMENT_H

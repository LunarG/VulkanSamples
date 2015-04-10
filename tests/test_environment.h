#ifndef TEST_ENVIRONMENT_H
#define TEST_ENVIRONMENT_H
#include "vktestbinding.h"
#include <vkWsiX11Ext.h>

namespace vk_testing {
class Environment : public ::testing::Environment {
public:
    Environment();

    bool parse_args(int argc, char **argv);

    virtual void SetUp();
    virtual void X11SetUp();
    virtual void TearDown();
    xcb_connection_t         *m_connection;
    xcb_screen_t             *m_screen;

    const std::vector<Device *> &devices() { return devs_; }
    Device &default_device() { return *(devs_[default_dev_]); }
    VkPhysicalGpu gpus[VK_MAX_PHYSICAL_GPUS];

private:
    VkApplicationInfo app_;
    int default_dev_;
    VkInstance inst;

    std::vector<Device *> devs_;
};
}
#endif // TEST_ENVIRONMENT_H

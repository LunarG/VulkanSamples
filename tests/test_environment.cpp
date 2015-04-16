#include "test_common.h"
#include "vktestbinding.h"
#include "test_environment.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace vk_testing {

Environment::Environment() :
    default_dev_(0)
{
    app_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_.pAppName = "vk_testing";
    app_.appVersion = 1;
    app_.pEngineName = "vk_testing";
    app_.engineVersion = 1;
    app_.apiVersion = VK_API_VERSION;
}

bool Environment::parse_args(int argc, char **argv)
{
    int i;

    for (i = 1; i < argc; i++) {
#define ARG(name) (strcmp(argv[i], name) == 0)
#define ARG_P(name) (i < argc - 1 && ARG(name))
        if (ARG_P("--gpu")) {
            default_dev_ = atoi(argv[++i]);
        } else {
            break;
        }
#undef ARG
#undef ARG_P
    }

    if (i < argc) {
        std::cout <<
            "invalid argument: " << argv[i] << "\n\n" <<
            "Usage: " << argv[0] << " <options>\n\n" <<
            "Options:\n"
            "  --gpu <n>  Use GPU<n> as the default GPU\n";

        return false;
    }

    return true;
}

void Environment::SetUp()
{

    uint32_t count;
    VkResult err;
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = 0;
    inst_info.ppEnabledExtensionNames = NULL;
    err = vkCreateInstance(&inst_info, &inst);
    ASSERT_EQ(VK_SUCCESS, err);
    err = vkEnumeratePhysicalDevices(inst, &count, NULL);
    ASSERT_EQ(VK_SUCCESS, err);
    ASSERT_LE(count, ARRAY_SIZE(gpus));
    err = vkEnumeratePhysicalDevices(inst, &count, gpus);
    ASSERT_EQ(VK_SUCCESS, err);
    ASSERT_GT(count, default_dev_);

    devs_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        devs_.push_back(new Device(gpus[i]));
        if (i == default_dev_) {
            devs_[i]->init();
            ASSERT_NE(true, devs_[i]->graphics_queues().empty());
        }
    }
}

void Environment::TearDown()
{
    // destroy devices first
    for (std::vector<Device *>::iterator it = devs_.begin(); it != devs_.end(); it++)
        delete *it;
    devs_.clear();

    vkDestroyInstance(inst);
}
} // vk_testing namespace

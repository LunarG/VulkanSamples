#include "test_common.h"
#include "xgltestbinding.h"
#include "test_environment.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace xgl_testing {

Environment::Environment() :
    m_connection(NULL), default_dev_(0)
{
    app_.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
    app_.pAppName = "xgl_testing";
    app_.appVersion = 1;
    app_.pEngineName = "xgl_testing";
    app_.engineVersion = 1;
    app_.apiVersion = XGL_API_VERSION;
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
    XGL_RESULT err;
    XGL_INSTANCE_CREATE_INFO inst_info = {};
    inst_info.sType = XGL_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = 0;
    inst_info.ppEnabledExtensionNames = NULL;
    err = xglCreateInstance(&inst_info, &inst);
    ASSERT_EQ(XGL_SUCCESS, err);
    err = xglEnumerateGpus(inst, ARRAY_SIZE(gpus), &count, gpus);
    ASSERT_EQ(XGL_SUCCESS, err);
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

void Environment::X11SetUp()
{

    uint32_t count;
    XGL_RESULT err;
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;
    XGL_INSTANCE_CREATE_INFO instInfo = {};
    instInfo.sType = XGL_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instInfo.pNext = NULL;
    instInfo.pAppInfo = &app_;
    instInfo.pAllocCb = NULL;
    instInfo.extensionCount = 0;
    instInfo.ppEnabledExtensionNames = NULL;

    err = xglCreateInstance(&instInfo, &inst);
    ASSERT_EQ(XGL_SUCCESS, err);
    err = xglEnumerateGpus(inst, ARRAY_SIZE(gpus), &count, gpus);
    ASSERT_EQ(XGL_SUCCESS, err);
    ASSERT_GT(count, default_dev_);

    m_connection = xcb_connect(NULL, &scr);

    setup = xcb_get_setup(m_connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    m_screen = iter.data;

    XGL_WSI_X11_CONNECTION_INFO connection_info = {};
    connection_info.pConnection = m_connection;
    connection_info.root = m_screen->root;
    connection_info.provider = 0;

    err = xglWsiX11AssociateConnection(gpus[0], &connection_info);
    assert(!err);


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
    if (m_connection)
        xcb_disconnect(m_connection);

    // destroy devices first
    for (std::vector<Device *>::iterator it = devs_.begin(); it != devs_.end(); it++)
        delete *it;
    devs_.clear();

    xglDestroyInstance(inst);
}
} // xgl_testing namespace

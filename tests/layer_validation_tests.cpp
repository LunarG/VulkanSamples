#include <vulkan.h>
#include <vkDbg.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vkrenderframework.h"

void VKAPI myDbgFunc(
    VK_DBG_MSG_TYPE     msgType,
    VkValidationLevel validationLevel,
    VkObject             srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData);

class ErrorMonitor {
public:
    ErrorMonitor()
    {
        m_msgType = VK_DBG_MSG_UNKNOWN;
    }
    void ClearState()
    {
        m_msgType = VK_DBG_MSG_UNKNOWN;
        m_msgString.clear();
    }
    VK_DBG_MSG_TYPE GetState(std::string *msgString)
    {
        *msgString = m_msgString;
        return m_msgType;
    }
    void SetState(VK_DBG_MSG_TYPE msgType, const char *msgString)
    {
        m_msgType = msgType;
        m_msgString.reserve(strlen(msgString));
        m_msgString = msgString;
    }

private:
    VK_DBG_MSG_TYPE        m_msgType;
    std::string             m_msgString;

};
void VKAPI myDbgFunc(
    VK_DBG_MSG_TYPE      msgType,
    VkValidationLevel    validationLevel,
    VkObject             srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData)
{
    if (msgType == VK_DBG_MSG_WARNING || msgType == VK_DBG_MSG_ERROR) {
        ErrorMonitor *errMonitor = (ErrorMonitor *)pUserData;
        errMonitor->SetState(msgType, pMsg);
    }
}
class VkLayerTest : public VkRenderFramework
{
public:
    VkResult BeginCommandBuffer(VkCommandBufferObj &cmdBuffer);
    VkResult EndCommandBuffer(VkCommandBufferObj &cmdBuffer);

protected:
        VkMemoryRefManager         m_memoryRefManager;
        ErrorMonitor               *m_errorMonitor;

    virtual void SetUp() {
        const char *extension_names[] = {"MemTracker", "ObjectTracker"};
        const std::vector<const char *> extensions(extension_names, extension_names + 2);

        size_t extSize = sizeof(uint32_t);
        uint32_t extCount = 0;
        VkResult U_ASSERT_ONLY err;
        err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &extCount);
        assert(!err);

        VkExtensionProperties extProp;
        extSize = sizeof(VkExtensionProperties);
        bool32_t extFound;

        for (uint32_t i = 0; i < extensions.size(); i++) {
            extFound = 0;
            for (uint32_t j = 0; j < extCount; j++) {
                err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, j, &extSize, &extProp);
                assert(!err);
                if (!strcmp(extensions[i], extProp.extName)) {
                   extFound = 1;
                   break;
                }
            }
            ASSERT_EQ(extFound, 1) << "ERROR: Cannot find extension named " << extensions[i] << " which is necessary to pass this test";
        }

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "layer_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        m_errorMonitor = new ErrorMonitor;
        InitFramework(extensions, myDbgFunc, m_errorMonitor);

    }

    virtual void TearDown() {
        // Clean up resources before we reset
        ShutdownFramework();
        delete m_errorMonitor;
    }
};
VkResult VkLayerTest::BeginCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VkResult result;

    result = cmdBuffer.BeginCommandBuffer();

    /*
     * For render test all drawing happens in a single render pass
     * on a single command buffer.
     */
    if (VK_SUCCESS == result) {
        cmdBuffer.BeginRenderPass(renderPass(), framebuffer());
    }

    return result;
}

VkResult VkLayerTest::EndCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VkResult result;

    cmdBuffer.EndRenderPass(renderPass());

    result = cmdBuffer.EndCommandBuffer();

    return result;
}

TEST_F(VkLayerTest, SubmitSignaledFence)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    BeginCommandBuffer(cmdBuffer);
    cmdBuffer.ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer(cmdBuffer);

    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    cmdBuffer.QueueCommandBuffer(testFence.obj());
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an err from using a fence in SIGNALED state in call to vkQueueSubmit";
    if (!strstr(msgString.c_str(),"submitted in SIGNALED state.  Fences must be reset before being submitted")) {
        FAIL() << "Error received was not VkQueueSubmit with fence in SIGNALED_STATE";
    }

}

TEST_F(VkLayerTest, ResetUnsignaledFence)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;

    ASSERT_NO_FATAL_FAILURE(InitState());
    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    VkFence fences[1] = {testFence.obj()};
    vkResetFences(m_device->device(), 1, fences);
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from submitting fence with UNSIGNALED state to vkResetFences";
    if (!strstr(msgString.c_str(),"submitted to VkResetFences in UNSIGNALED STATE")) {
        FAIL() << "Error received was not VkResetFences with fence in UNSIGNALED_STATE";
    }

}

TEST_F(VkLayerTest, WaitForUnsubmittedFence)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;

    ASSERT_NO_FATAL_FAILURE(InitState());
    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    vkGetFenceStatus(m_device->device(),testFence.obj());
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error asking for status of unsubmitted fence";
    if (!strstr(msgString.c_str(),"Status Requested for Unsubmitted Fence")) {
        FAIL() << "Error received was not Status Requested for Unsubmitted Fence";
    }

    VkFence fences[1] = {testFence.obj()};
    m_errorMonitor->ClearState();
    vkWaitForFences(m_device->device(), 1, fences, VK_TRUE, 0);
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error for waiting for unsubmitted fence";
    if (!strstr(msgString.c_str(),"Waiting for Unsubmitted Fence")) {
        FAIL() << "Error received was not Waiting for Unsubmitted Fence";
    }
}

TEST_F(VkLayerTest, GetObjectInfoMismatchedType)
{
    VkEventCreateInfo event_info;
    VkEvent event;
    VkMemoryRequirements mem_req;
    size_t data_size = sizeof(mem_req);
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;
    VkResult err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(device(), &event_info, &event);
    ASSERT_VK_SUCCESS(err);
    m_errorMonitor->ClearState();
    err = vkGetObjectInfo(device(), VK_OBJECT_TYPE_IMAGE, event, VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from mismatched types in vkGetObjectInfo";
    if (!strstr(msgString.c_str(),"does not match designated type")) {
        FAIL() << "Error received was not event does not match designated type image";
    }

}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    VkTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    VkTestFramework::Finish();
    return result;
}

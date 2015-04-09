#include <vulkan.h>
#include <vkDbg.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vkrenderframework.h"

void VKAPI myDbgFunc(
    VK_DBG_MSG_TYPE     msgType,
    VK_VALIDATION_LEVEL validationLevel,
    VK_BASE_OBJECT      srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData);

class ErrorMonitor {
public:
    ErrorMonitor(VK_INSTANCE inst)
    {
        vkDbgRegisterMsgCallback(inst, myDbgFunc, this);
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
    VK_DBG_MSG_TYPE     msgType,
    VK_VALIDATION_LEVEL validationLevel,
    VK_BASE_OBJECT      srcObject,
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
    VK_RESULT BeginCommandBuffer(VkCommandBufferObj &cmdBuffer);
    VK_RESULT EndCommandBuffer(VkCommandBufferObj &cmdBuffer);

protected:
        VkMemoryRefManager         m_memoryRefManager;
        ErrorMonitor               *m_errorMonitor;

    virtual void SetUp() {

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "layer_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        InitFramework();
        m_errorMonitor = new ErrorMonitor(inst);
    }

    virtual void TearDown() {
        // Clean up resources before we reset
        ShutdownFramework();
        delete m_errorMonitor;
    }
};
VK_RESULT VkLayerTest::BeginCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VK_RESULT result;

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

VK_RESULT VkLayerTest::EndCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VK_RESULT result;

    cmdBuffer.EndRenderPass(renderPass());

    result = cmdBuffer.EndCommandBuffer();

    return result;
}

TEST_F(VkLayerTest, SubmitSignaledFence)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;
    VK_FENCE_CREATE_INFO fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // Verifiy that the appropriate layer is loaded

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
    VK_FENCE_CREATE_INFO fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;

    // Verifiy that the appropriate layer is loaded

    ASSERT_NO_FATAL_FAILURE(InitState());
    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    VK_FENCE fences[1] = {testFence.obj()};
    vkResetFences(m_device->device(), 1, fences);
    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from submitting fence with UNSIGNALED state to vkResetFences";
    if (!strstr(msgString.c_str(),"submitted to VkResetFences in UNSIGNALED STATE")) {
        FAIL() << "Error received was not VkResetFences with fence in UNSIGNALED_STATE";
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

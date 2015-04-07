
#include <xgl.h>
#include <xglDbg.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "xglrenderframework.h"

void XGLAPI myDbgFunc(
    XGL_DBG_MSG_TYPE     msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData);

class ErrorMonitor {
public:
    ErrorMonitor(XGL_INSTANCE inst)
    {
        xglDbgRegisterMsgCallback(inst, myDbgFunc, this);
        m_msgType = XGL_DBG_MSG_UNKNOWN;
    }
    void ClearState()
    {
        m_msgType = XGL_DBG_MSG_UNKNOWN;
        m_msgString.clear();
    }
    XGL_DBG_MSG_TYPE GetState(std::string *msgString)
    {
        *msgString = m_msgString;
        return m_msgType;
    }
    void SetState(XGL_DBG_MSG_TYPE msgType, const char *msgString)
    {
        m_msgType = msgType;
        m_msgString = *msgString;
    }

private:
    XGL_DBG_MSG_TYPE        m_msgType;
    std::string             m_msgString;

};
void XGLAPI myDbgFunc(
    XGL_DBG_MSG_TYPE     msgType,
    XGL_VALIDATION_LEVEL validationLevel,
    XGL_BASE_OBJECT      srcObject,
    size_t               location,
    int32_t              msgCode,
    const char*          pMsg,
    void*                pUserData)
{
    ErrorMonitor *errMonitor = (ErrorMonitor *)pUserData;
    errMonitor->SetState(msgType, pMsg);
}
class XglLayerTest : public XglRenderFramework
{
public:
    XGL_RESULT BeginCommandBuffer(XglCommandBufferObj &cmdBuffer);
    XGL_RESULT EndCommandBuffer(XglCommandBufferObj &cmdBuffer);

protected:
        XglMemoryRefManager         m_memoryRefManager;
        ErrorMonitor                *m_errorMonitor;

    virtual void SetUp() {

        this->app_info.sType = XGL_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "layer_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = XGL_API_VERSION;

        InitFramework();
        m_errorMonitor = new ErrorMonitor(inst);
    }

    virtual void TearDown() {
        // Clean up resources before we reset
        delete m_errorMonitor;
        ShutdownFramework();
    }
};
XGL_RESULT XglLayerTest::BeginCommandBuffer(XglCommandBufferObj &cmdBuffer)
{
    XGL_RESULT result;

    result = cmdBuffer.BeginCommandBuffer();

    /*
     * For render test all drawing happens in a single render pass
     * on a single command buffer.
     */
    if (XGL_SUCCESS == result) {
        cmdBuffer.BeginRenderPass(renderPass(), framebuffer());
    }

    return result;
}

XGL_RESULT XglLayerTest::EndCommandBuffer(XglCommandBufferObj &cmdBuffer)
{
    XGL_RESULT result;

    cmdBuffer.EndRenderPass(renderPass());

    result = cmdBuffer.EndCommandBuffer();

    return result;
}

TEST_F(XglLayerTest, UseSignaledFence)
{
    xgl_testing::Fence testFence;
    XGL_DBG_MSG_TYPE msgType;
    std::string msgString;
    const XGL_FENCE_CREATE_INFO fenceInfo = {
        .sType = XGL_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = NULL,
        .flags = XGL_FENCE_CREATE_SIGNALED_BIT,
    };

    // Register error callback to catch errors and record parameters

    // Verifiy that the appropriate layer is loaded

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    XglCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    cmdBuffer.begin();
    cmdBuffer.ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    cmdBuffer.end();

    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    cmdBuffer.QueueCommandBuffer();
    msgType = m_errorMonitor->GetState(&msgString);

}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    XglTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    XglTestFramework::Finish();
    return result;
}

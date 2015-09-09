#include <vulkan.h>
#include "vk_debug_report_lunarg.h"
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vkrenderframework.h"
#include "vk_layer_config.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

#define MEM_TRACKER_TESTS 1
#define OBJ_TRACKER_TESTS 1
#define DRAW_STATE_TESTS 1
#define THREADING_TESTS 1
#define SHADER_CHECKER_TESTS 1

//--------------------------------------------------------------------------------------
// Mesh and VertexFormat Data
//--------------------------------------------------------------------------------------
struct Vertex
{
    float posX, posY, posZ, posW;    // Position data
    float r, g, b, a;                // Color
};

#define XYZ1(_x_, _y_, _z_)         (_x_), (_y_), (_z_), 1.f

typedef enum _BsoFailSelect {
    BsoFailNone                     = 0x00000000,
    BsoFailLineWidth                = 0x00000001,
    BsoFailDepthBias                = 0x00000002,
    BsoFailViewport                 = 0x00000004,
    BsoFailBlend                    = 0x00000008,
    BsoFailDepthBounds              = 0x00000010,
    BsoFailStencil                  = 0x00000020,
} BsoFailSelect;

struct vktriangle_vs_uniform {
    // Must start with MVP
    float   mvp[4][4];
    float   position[3][4];
    float   color[3][4];
};

static const char bindStateVertShaderText[] =
        "#version 130\n"
        "vec2 vertices[3];\n"
        "void main() {\n"
        "      vertices[0] = vec2(-1.0, -1.0);\n"
        "      vertices[1] = vec2( 1.0, -1.0);\n"
        "      vertices[2] = vec2( 0.0,  1.0);\n"
        "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
        "}\n";

static const char bindStateFragShaderText[] =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location = 0) out vec4 uFragColor;\n"
        "void main(){\n"
        "   uFragColor = vec4(0,1,0,1);\n"
        "}\n";

static void myDbgFunc(
    VkFlags                             msgFlags,
    VkDbgObjectType                     objType,
    uint64_t                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

class ErrorMonitor {
public:
    ErrorMonitor()
    {
        test_platform_thread_create_mutex(&m_mutex);
        test_platform_thread_lock_mutex(&m_mutex);
        m_msgFlags = VK_DBG_REPORT_INFO_BIT;
        m_bailout = NULL;
        test_platform_thread_unlock_mutex(&m_mutex);
    }
    void ClearState()
    {
        test_platform_thread_lock_mutex(&m_mutex);
        m_msgFlags = VK_DBG_REPORT_INFO_BIT;
        m_msgString.clear();
        test_platform_thread_unlock_mutex(&m_mutex);
    }
    VkFlags GetState(std::string *msgString)
    {
        test_platform_thread_lock_mutex(&m_mutex);
        *msgString = m_msgString;
        test_platform_thread_unlock_mutex(&m_mutex);
        return m_msgFlags;
    }
    void SetState(VkFlags msgFlags, const char *msgString)
    {
        test_platform_thread_lock_mutex(&m_mutex);
        if (m_bailout != NULL) {
            *m_bailout = true;
        }
        m_msgFlags = msgFlags;
        m_msgString.reserve(strlen(msgString));
        m_msgString = msgString;
        test_platform_thread_unlock_mutex(&m_mutex);
    }
    void SetBailout(bool *bailout)
    {
        m_bailout = bailout;
    }

private:
    VkFlags                    m_msgFlags;
    std::string                m_msgString;
    test_platform_thread_mutex m_mutex;
    bool*                      m_bailout;
};

static void myDbgFunc(
    VkFlags                    msgFlags,
    VkDbgObjectType            objType,
    uint64_t                   srcObject,
    size_t                     location,
    int32_t                    msgCode,
    const char*                pLayerPrefix,
    const char*                pMsg,
    void*                      pUserData)
{
    if (msgFlags & (VK_DBG_REPORT_WARN_BIT | VK_DBG_REPORT_ERROR_BIT)) {
        ErrorMonitor *errMonitor = (ErrorMonitor *)pUserData;
        errMonitor->SetState(msgFlags, pMsg);
    }
}

class VkLayerTest : public VkRenderFramework
{
public:
    VkResult BeginCommandBuffer(VkCommandBufferObj &cmdBuffer);
    VkResult EndCommandBuffer(VkCommandBufferObj &cmdBuffer);
    void VKTriangleTest(const char *vertShaderText, const char *fragShaderText, BsoFailSelect failMask);
    void GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask);
    void GenericDrawPreparation(VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet,  BsoFailSelect failMask)
             { GenericDrawPreparation(m_cmdBuffer, pipelineobj, descriptorSet, failMask); }

    /* Convenience functions that use built-in command buffer */
    VkResult BeginCommandBuffer() { return BeginCommandBuffer(*m_cmdBuffer); }
    VkResult EndCommandBuffer() { return EndCommandBuffer(*m_cmdBuffer); }
    void Draw(uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
        { m_cmdBuffer->Draw(firstVertex, vertexCount, firstInstance, instanceCount); }
    void DrawIndexed(uint32_t firstVertex, uint32_t vertexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
        { m_cmdBuffer->DrawIndexed(firstVertex, vertexCount, vertexOffset,firstInstance, instanceCount); }
    void QueueCommandBuffer() { m_cmdBuffer->QueueCommandBuffer(); }
    void QueueCommandBuffer(const VkFence& fence) { m_cmdBuffer->QueueCommandBuffer(fence); }
    void BindVertexBuffer(VkConstantBufferObj *vertexBuffer, VkDeviceSize offset, uint32_t binding)
        { m_cmdBuffer->BindVertexBuffer(vertexBuffer, offset, binding); }
    void BindIndexBuffer(VkIndexBufferObj *indexBuffer, VkDeviceSize offset)
        { m_cmdBuffer->BindIndexBuffer(indexBuffer, offset); }
protected:
        ErrorMonitor               *m_errorMonitor;

    virtual void SetUp() {
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> device_layer_names;
        std::vector<const char *> instance_extension_names;
        std::vector<const char *> device_extension_names;

        instance_extension_names.push_back(VK_DEBUG_REPORT_EXTENSION_NAME);
        /*
         * Since CreateDbgMsgCallback is an instance level extension call
         * any extension / layer that utilizes that feature also needs
         * to be enabled at create instance time.
         */
	// Use Threading layer first to protect others from ThreadCmdBufferCollision test
        instance_layer_names.push_back("Threading");
        instance_layer_names.push_back("ObjectTracker");
        instance_layer_names.push_back("MemTracker");
        instance_layer_names.push_back("DrawState");
        instance_layer_names.push_back("ShaderChecker");

        device_layer_names.push_back("Threading");
        device_layer_names.push_back("ObjectTracker");
        device_layer_names.push_back("MemTracker");
        device_layer_names.push_back("DrawState");
        device_layer_names.push_back("ShaderChecker");

        this->app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        this->app_info.pNext = NULL;
        this->app_info.pAppName = "layer_tests";
        this->app_info.appVersion = 1;
        this->app_info.pEngineName = "unittest";
        this->app_info.engineVersion = 1;
        this->app_info.apiVersion = VK_API_VERSION;

        m_errorMonitor = new ErrorMonitor;
        InitFramework(instance_layer_names, device_layer_names,
                      instance_extension_names, device_extension_names,
                      myDbgFunc, m_errorMonitor);
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
    if (VK_SUCCESS == result && renderPass()) {
        cmdBuffer.BeginRenderPass(renderPassBeginInfo());
    }

    return result;
}

VkResult VkLayerTest::EndCommandBuffer(VkCommandBufferObj &cmdBuffer)
{
    VkResult result;

    if (renderPass()) {
        cmdBuffer.EndRenderPass();
    }

    result = cmdBuffer.EndCommandBuffer();

    return result;
}

void VkLayerTest::VKTriangleTest(const char *vertShaderText, const char *fragShaderText, BsoFailSelect failMask)
{
    // Create identity matrix
    int i;
    struct vktriangle_vs_uniform data;

    glm::mat4 Projection = glm::mat4(1.0f);
    glm::mat4 View       = glm::mat4(1.0f);
    glm::mat4 Model      = glm::mat4(1.0f);
    glm::mat4 MVP        = Projection * View * Model;
    const int matrixSize = sizeof(MVP);
    const int bufSize    = sizeof(vktriangle_vs_uniform) / sizeof(float);

    memcpy(&data.mvp, &MVP[0][0], matrixSize);

    static const Vertex tri_data[] =
    {
        { XYZ1( -1, -1, 0 ), XYZ1( 1.f, 0.f, 0.f ) },
        { XYZ1(  1, -1, 0 ), XYZ1( 0.f, 1.f, 0.f ) },
        { XYZ1(  0,  1, 0 ), XYZ1( 0.f, 0.f, 1.f ) },
    };

    for (i=0; i<3; i++) {
        data.position[i][0] = tri_data[i].posX;
        data.position[i][1] = tri_data[i].posY;
        data.position[i][2] = tri_data[i].posZ;
        data.position[i][3] = tri_data[i].posW;
        data.color[i][0]    = tri_data[i].r;
        data.color[i][1]    = tri_data[i].g;
        data.color[i][2]    = tri_data[i].b;
        data.color[i][3]    = tri_data[i].a;
    }

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());

    VkConstantBufferObj constantBuffer(m_device, bufSize*2, sizeof(float), (const void*) &data);

    VkShaderObj vs(m_device,vertShaderText,VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj ps(m_device,fragShaderText, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipelineobj(m_device);
    pipelineobj.AddColorAttachment();
    pipelineobj.AddShader(&vs);
    pipelineobj.AddShader(&ps);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, constantBuffer);

    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    ASSERT_VK_SUCCESS(BeginCommandBuffer());

    GenericDrawPreparation(pipelineobj, descriptorSet, failMask);

    // render triangle
    Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer();

    QueueCommandBuffer();
}

void VkLayerTest::GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask)
{
    if (m_depthStencil->Initialized()) {
        cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, m_depthStencil);
    } else {
        cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    }

    cmdBuffer->PrepareAttachments();
    if ((failMask & BsoFailLineWidth) != BsoFailLineWidth) {
        cmdBuffer->BindDynamicLineWidthState(m_stateLineWidth);
    }
    if ((failMask & BsoFailDepthBias) != BsoFailDepthBias) {
        cmdBuffer->BindDynamicDepthBiasState(m_stateDepthBias);
    }
    if ((failMask & BsoFailViewport) != BsoFailViewport) {
        cmdBuffer->BindDynamicViewportState(m_stateViewport);
    }
    if ((failMask & BsoFailBlend) != BsoFailBlend) {
        cmdBuffer->BindDynamicBlendState(m_stateBlend);
    }
    if ((failMask & BsoFailDepthBounds) != BsoFailDepthBounds) {
        cmdBuffer->BindDynamicDepthBoundsState(m_stateDepthBounds);
    }
    if ((failMask & BsoFailStencil) != BsoFailStencil) {
        cmdBuffer->BindDynamicStencilState(m_stateStencil);
    }
    // Make sure depthWriteEnable is set so that Depth fail test will work correctly
    // Make sure stencilTestEnable is set so that Stencil fail test will work correctly
    VkStencilOpState stencil = {};
    stencil.stencilFailOp = VK_STENCIL_OP_KEEP;
        stencil.stencilPassOp = VK_STENCIL_OP_KEEP;
        stencil.stencilDepthFailOp = VK_STENCIL_OP_KEEP;
        stencil.stencilCompareOp = VK_COMPARE_OP_NEVER;

    VkPipelineDepthStencilStateCreateInfo ds_ci = {};
    ds_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        ds_ci.pNext = NULL;
        ds_ci.depthTestEnable = VK_FALSE;
        ds_ci.depthWriteEnable = VK_TRUE;
        ds_ci.depthCompareOp = VK_COMPARE_OP_NEVER;
        ds_ci.depthBoundsTestEnable = VK_FALSE;
        ds_ci.stencilTestEnable = VK_TRUE;
        ds_ci.front = stencil;
        ds_ci.back = stencil;

    pipelineobj.SetDepthStencil(&ds_ci);
    descriptorSet.CreateVKDescriptorSet(cmdBuffer);
    VkResult err = pipelineobj.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    ASSERT_VK_SUCCESS(err);
    cmdBuffer->BindPipeline(pipelineobj);
    cmdBuffer->BindDescriptorSet(descriptorSet);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************
#if MEM_TRACKER_TESTS
TEST_F(VkLayerTest, CallResetCmdBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VkFlags msgFlags;
    std::string msgString;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    ASSERT_NO_FATAL_FAILURE(InitState());

    VkMemoryPropertyFlags reqs = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    vk_testing::Buffer buffer;
    buffer.init_as_dst(*m_device, (VkDeviceSize)20, reqs);

    BeginCommandBuffer();
    m_cmdBuffer->FillBuffer(buffer.handle(), 0, 4, 0x11111111);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    err = vkQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer->handle(), testFence.handle());
    ASSERT_VK_SUCCESS( err );

    m_errorMonitor->ClearState();
    // Introduce failure by calling begin again before checking fence
    vkResetCommandBuffer(m_cmdBuffer->handle(), 0);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an err after calling ResetCommandBuffer on an active Command Buffer";
    if (!strstr(msgString.c_str(),"Resetting CB")) {
        FAIL() << "Error received was not 'Resetting CB (0xaddress) before it has completed. You must check CB flag before'";
    }
}

TEST_F(VkLayerTest, CallBeginCmdBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VkFlags msgFlags;
    std::string msgString;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    m_cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    err = vkQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer->handle(), testFence.handle());
    ASSERT_VK_SUCCESS( err );

    m_errorMonitor->ClearState();
    // Introduce failure by calling begin again before checking fence
    BeginCommandBuffer();

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an err after calling BeginCommandBuffer on an active Command Buffer";
    if (!strstr(msgString.c_str(),"Calling vkBeginCommandBuffer() on active CB")) {
        FAIL() << "Error received was not 'Calling vkBeginCommandBuffer() on an active CB (0xaddress) before it has completed'";
    }
}

TEST_F(VkLayerTest, MapMemWithoutHostVisibleBit)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory        mem;
    VkMemoryRequirements  mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;

    VkImageCreateInfo image_create_info = {};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = tex_format;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arraySize = 1;
        image_create_info.samples = 1;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.flags = 0;
   
    VkMemoryAllocInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = 0;
        // Introduce failure, do NOT set memProps to VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        mem_alloc.memoryTypeIndex = 1;

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetImageMemoryRequirements(m_device->device(),
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;

	err = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT);
    if(err != VK_SUCCESS) // If we can't find any unmappable memory this test doesn't make sense
        return;

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // Map memory as if to initialize the image
    void *mappedAddress = NULL;
    err = vkMapMemory(m_device->device(), mem, 0, 0, 0, &mappedAddress);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error while tring to map memory not visible to CPU";
    if (!strstr(msgString.c_str(),"Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT")) {
        FAIL() << "Error received did not match expected error message from vkMapMemory in MemTracker";
    }
}

TEST_F(VkLayerTest, BindInvalidMemory)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory        mem;
    VkMemoryRequirements  mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;

    VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = tex_format;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arraySize = 1;
        image_create_info.samples = 1;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetImageMemoryRequirements(m_device->device(),
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;

    err = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory before binding
    vkFreeMemory(m_device->device(), mem);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    // This may very well return an error.
    (void)err;

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error while tring to bind a freed memory object";
    if (!strstr(msgString.c_str(),"couldn't find info for mem obj")) {
        FAIL() << "Error received did not match expected error message from BindObjectMemory in MemTracker";
    }
}

// TODO : Is this test still valid. Not sure it is with updates to memory binding model
//  Verify and delete the test of fix the check
//TEST_F(VkLayerTest, FreeBoundMemory)
//{
//    VkFlags         msgFlags;
//    std::string     msgString;
//    VkResult        err;
//
//    ASSERT_NO_FATAL_FAILURE(InitState());
//    m_errorMonitor->ClearState();
//
//    // Create an image, allocate memory, free it, and then try to bind it
//    VkImage               image;
//    VkDeviceMemory        mem;
//    VkMemoryRequirements  mem_reqs;
//
//    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
//    const int32_t  tex_width       = 32;
//    const int32_t  tex_height      = 32;
//
//    const VkImageCreateInfo image_create_info = {
//        .sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
//        .pNext           = NULL,
//        .imageType       = VK_IMAGE_TYPE_2D,
//        .format          = tex_format,
//        .extent          = { tex_width, tex_height, 1 },
//        .mipLevels       = 1,
//        .arraySize       = 1,
//        .samples         = 1,
//        .tiling          = VK_IMAGE_TILING_LINEAR,
//        .usage           = VK_IMAGE_USAGE_SAMPLED_BIT,
//        .flags           = 0,
//    };
//    VkMemoryAllocInfo mem_alloc = {
//        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
//        .pNext           = NULL,
//        .allocationSize  = 0,
//        .memoryTypeIndex = 0,
//    };
//
//    err = vkCreateImage(m_device->device(), &image_create_info, &image);
//    ASSERT_VK_SUCCESS(err);
//
//    err = vkGetImageMemoryRequirements(m_device->device(),
//                          image,
//                          &mem_reqs);
//    ASSERT_VK_SUCCESS(err);
//
//    mem_alloc.allocationSize = mem_reqs.size;
//
//    err = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
//    ASSERT_VK_SUCCESS(err);
//
//    // allocate memory
//    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
//    ASSERT_VK_SUCCESS(err);
//
//    // Bind memory to Image object
//    err = vkBindImageMemory(m_device->device(), image, mem, 0);
//    ASSERT_VK_SUCCESS(err);
//
//    // Introduce validation failure, free memory while still bound to object
//    vkFreeMemory(m_device->device(), mem);
//    ASSERT_VK_SUCCESS(err);
//
//    msgFlags = m_errorMonitor->GetState(&msgString);
//    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an warning while tring to free bound memory";
//    if (!strstr(msgString.c_str(),"Freeing memory object while it still has references")) {
//        FAIL() << "Warning received did not match expected message from freeMemObjInfo  in MemTracker";
//    }
//}

TEST_F(VkLayerTest, RebindMemory)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory        mem1;
    VkDeviceMemory        mem2;
    VkMemoryRequirements  mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;

    VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = tex_format;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arraySize = 1;
        image_create_info.samples = 1;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

    // Introduce failure, do NOT set memProps to VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
    mem_alloc.memoryTypeIndex = 1;
    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetImageMemoryRequirements(m_device->device(),
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;
    err = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // allocate 2 memory objects
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem1);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem2);
    ASSERT_VK_SUCCESS(err);

    // Bind first memory object to Image object
    err = vkBindImageMemory(m_device->device(), image, mem1, 0);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, try to bind a different memory object to the same image object
    err = vkBindImageMemory(m_device->device(), image, mem2, 0);
    ASSERT_VK_SUCCESS(err);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error while tring to rebind an object";
    if (!strstr(msgString.c_str(),"which has already been bound to mem object")) {
        FAIL() << "Error received did not match expected message when rebinding memory to an object";
    }
}

TEST_F(VkLayerTest, BindMemoryToDestroyedObject)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image object, allocate memory, destroy the object and then try to bind it
    VkImage               image;
    VkDeviceMemory        mem;
    VkMemoryRequirements  mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;

    VkImageCreateInfo image_create_info = {};
        image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image_create_info.pNext = NULL;
        image_create_info.imageType = VK_IMAGE_TYPE_2D;
        image_create_info.format = tex_format;
        image_create_info.extent.width = tex_width;
        image_create_info.extent.height = tex_height;
        image_create_info.extent.depth = 1;
        image_create_info.mipLevels = 1;
        image_create_info.arraySize = 1;
        image_create_info.samples = 1;
        image_create_info.tiling = VK_IMAGE_TILING_LINEAR;
        image_create_info.usage = VK_IMAGE_USAGE_SAMPLED_BIT;
        image_create_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
        mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
        mem_alloc.pNext = NULL;
        mem_alloc.allocationSize = 0;
        mem_alloc.memoryTypeIndex = 0;

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetImageMemoryRequirements(m_device->device(),
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;
    err = m_device->phy().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, destroy Image object before binding
    vkDestroyImage(m_device->device(), image);
    ASSERT_VK_SUCCESS(err);

    // Now Try to bind memory to this destroyed object
    err = vkBindImageMemory(m_device->device(), image, mem, 0);
    // This may very well return an error.
    (void) err;

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error while binding memory to a destroyed object";
    if (!strstr(msgString.c_str(),"that's not in global list")) {
        FAIL() << "Error received did not match expected error message from updateObjectBinding in MemTracker";
    }
}

TEST_F(VkLayerTest, SubmitSignaledFence)
{
    vk_testing::Fence testFence;
    VkFlags msgFlags;
    std::string msgString;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    BeginCommandBuffer();
    m_cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    QueueCommandBuffer(testFence.handle());
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an err from using a fence in SIGNALED state in call to vkQueueSubmit";
    if (!strstr(msgString.c_str(),"submitted in SIGNALED state.  Fences must be reset before being submitted")) {
        FAIL() << "Error received was not 'VkQueueSubmit with fence in SIGNALED_STATE'";
    }

}

TEST_F(VkLayerTest, ResetUnsignaledFence)
{
    vk_testing::Fence testFence;
    VkFlags msgFlags;
    std::string msgString;
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;

    ASSERT_NO_FATAL_FAILURE(InitState());
    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    VkFence fences[1] = {testFence.handle()};
    vkResetFences(m_device->device(), 1, fences);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from submitting fence with UNSIGNALED state to vkResetFences";
    if (!strstr(msgString.c_str(),"submitted to VkResetFences in UNSIGNALED STATE")) {
        FAIL() << "Error received was not 'VkResetFences with fence in UNSIGNALED_STATE'";
    }

}

/* TODO: Update for changes due to bug-14075 tiling across render passes */
#if 0
TEST_F(VkLayerTest, InvalidUsageBits)
{
    // Initiate Draw w/o a PSO bound
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    VkCommandBufferObj cmdBuffer(m_device);
    BeginCommandBuffer();

    const VkExtent3D e3d = {
        .width = 128,
        .height = 128,
        .depth = 1,
    };
    const VkImageCreateInfo ici = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext = NULL,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = VK_FORMAT_D32_SFLOAT_S8_UINT,
        .extent = e3d,
        .mipLevels = 1,
        .arraySize = 1,
        .samples = 1,
        .tiling = VK_IMAGE_TILING_LINEAR,
        .usage = 0, // Not setting VK_IMAGE_USAGE_DEPTH_STENCIL_BIT
        .flags = 0,
    };

    VkImage dsi;
    vkCreateImage(m_device->device(), &ici, &dsi);
    VkDepthStencilView dsv;
    const VkDepthStencilViewCreateInfo dsvci = {
        .sType = VK_STRUCTURE_TYPE_DEPTH_STENCIL_VIEW_CREATE_INFO,
        .pNext = NULL,
        .image = dsi,
        .mipLevel = 0,
        .baseArraySlice = 0,
        .arraySize = 1,
        .flags = 0,
    };
    vkCreateDepthStencilView(m_device->device(), &dsvci, &dsv);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after attempting to create DSView w/ image lacking USAGE_DS_BIT flag";
    if (!strstr(msgString.c_str(),"Invalid usage flag for image ")) {
        FAIL() << "Error received was not 'Invalid usage flag for image...'";
    }
}
#endif
#endif
#if OBJ_TRACKER_TESTS
TEST_F(VkLayerTest, LineWidthStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a line width state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailLineWidth);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Line Width State Object";
    if (!strstr(msgString.c_str(),"Line width object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Line Width object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, DepthBiasStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a depth bias state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthBias);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Depth Bias State Object";
    if (!strstr(msgString.c_str(),"Depth bias object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Depth bias object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, ViewportStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a viewport state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailViewport);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Viewport State Object";
    if (!strstr(msgString.c_str(),"Viewport object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Viewport object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, BlendStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a blend state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailBlend);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Blend State Object";
    if (!strstr(msgString.c_str(),"Blend object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Blend object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, DepthBoundsStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a depth bounds state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthBounds);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Depth Bounds State Object";
    if (!strstr(msgString.c_str(),"Depth bounds object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Depth bounds object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, StencilStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a stencil state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailStencil);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Not Binding a Stencil State Object";
    if (!strstr(msgString.c_str(),"Stencil object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Stencil object not bound to this command buffer'";
    }
}
#endif
#if DRAW_STATE_TESTS
TEST_F(VkLayerTest, CmdBufferTwoSubmits)
{
    vk_testing::Fence testFence;
    VkFlags msgFlags;
    std::string msgString;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.pNext = NULL;
    fenceInfo.flags = 0;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    // We luck out b/c by default the framework creates CB w/ the VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT set
    BeginCommandBuffer();
    m_cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer();

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    err = vkQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer->handle(), testFence.handle());
    ASSERT_VK_SUCCESS( err );

    m_errorMonitor->ClearState();
    // Cause validation error by re-submitting cmd buffer that should only be submitted once
    err = vkQueueSubmit( m_device->m_queue, 1, &m_cmdBuffer->handle(), testFence.handle());
    ASSERT_VK_SUCCESS( err );

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an err after re-submitting Command Buffer that was created with one-time submit flag";
    if (!strstr(msgString.c_str(),"was begun w/ VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT set, but has been submitted")) {
        FAIL() << "Error received was not 'CB (0xaddress) was created w/ VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT set...'";
    }
}


TEST_F(VkLayerTest, BindPipelineNoRenderPass)
{
    // Initiate Draw w/o a PSO bound
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    BeginCommandBuffer();
    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding pipeline to CmdBuffer w/o active RenderPass";
    if (!strstr(msgString.c_str(),"Incorrectly binding graphics pipeline ")) {
        FAIL() << "Error received was not 'Incorrectly binding graphics pipeline (0xbaadb1be) without an active RenderPass'";
    }
}

TEST_F(VkLayerTest, InvalidDescriptorPool)
{
    // TODO : Simple check for bad object should be added to ObjectTracker to catch this case
    //   The DS check for this is after driver has been called to validate DS internal data struct
    // Attempt to clear DS Pool with bad object
/*    VkFlags msgFlags;
    std::string msgString;
    VkDescriptorPool badPool = (VkDescriptorPool)0xbaad6001;
    vkResetDescriptorPool(device(), badPool);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an error from Resetting an invalid DescriptorPool Object";
    if (!strstr(msgString.c_str(),"Unable to find pool node for pool 0xbaad6001 specified in vkResetDescriptorPool() call")) {
        FAIL() << "Error received was note 'Unable to find pool node for pool 0xbaad6001 specified in vkResetDescriptorPool() call'";
    }*/
}

TEST_F(VkLayerTest, InvalidDescriptorSet)
{
    // TODO : Simple check for bad object should be added to ObjectTracker to catch this case
    //   The DS check for this is after driver has been called to validate DS internal data struct
    // Create a valid cmd buffer
    // call vkCmdBindDescriptorSets w/ false DS
}

TEST_F(VkLayerTest, InvalidDescriptorSetLayout)
{
    // TODO : Simple check for bad object should be added to ObjectTracker to catch this case
    //   The DS check for this is after driver has been called to validate DS internal data struct
}

TEST_F(VkLayerTest, InvalidPipeline)
{
    // TODO : Simple check for bad object should be added to ObjectTracker to catch this case
    //   The DS check for this is after driver has been called to validate DS internal data struct
    // Create a valid cmd buffer
    // call vkCmdBindPipeline w/ false Pipeline
//    VkFlags         msgFlags;
//    std::string     msgString;
//
//    ASSERT_NO_FATAL_FAILURE(InitState());
//    m_errorMonitor->ClearState();
//    VkCommandBufferObj cmdBuffer(m_device);
//    BeginCommandBuffer();
//    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;
//    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);
//    msgFlags = m_errorMonitor->GetState(&msgString);
//    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding invalid pipeline to CmdBuffer";
//    if (!strstr(msgString.c_str(),"Attempt to bind Pipeline ")) {
//        FAIL() << "Error received was not 'Attempt to bind Pipeline 0xbaadb1be that doesn't exist!'";
//    }
}

TEST_F(VkLayerTest, DescriptorSetNotUpdated)
{
    // Create and update CmdBuffer then call QueueSubmit w/o calling End on CmdBuffer
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT, this); //  TODO - We shouldn't need a fragment shader
                                                                                       // but add it to be able to run on more devices

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    vkCmdBindDescriptorSets(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptorSet, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT) << "Did not warn after binding a DescriptorSet that was never updated.";
    if (!strstr(msgString.c_str()," bound but it was never updated. ")) {
        FAIL() << "Error received was not 'DS <blah> bound but it was never updated. You may want to either update it or not bind it.'";
    }
}

TEST_F(VkLayerTest, NoBeginCmdBuffer)
{
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    VkCommandBufferObj cmdBuffer(m_device, m_cmdPool);
    // Call EndCommandBuffer() w/o calling BeginCommandBuffer()
    vkEndCommandBuffer(cmdBuffer.GetBufferHandle());
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after ending a CmdBuffer w/o calling BeginCommandBuffer()";
    if (!strstr(msgString.c_str(),"You must call vkBeginCommandBuffer() before this call to ")) {
        FAIL() << "Error received was not 'You must call vkBeginCommandBuffer() before this call to vkEndCommandBuffer()'";
    }
}

TEST_F(VkLayerTest, PrimaryCmdBufferFramebufferAndRenderpass)
{
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Calls CreateCommandBuffer
    VkCommandBufferObj cmdBuffer(m_device, m_cmdPool);

    // Force the failure by setting the Renderpass and Framebuffer fields with (fake) data
    VkCmdBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                         VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;
    cmd_buf_info.renderPass = (VkRenderPass)0xcadecade;
    cmd_buf_info.framebuffer = (VkFramebuffer)0xcadecade;


    // The error should be caught by validation of the BeginCommandBuffer call
    vkBeginCommandBuffer(cmdBuffer.GetBufferHandle(), &cmd_buf_info);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error passing a non-NULL Framebuffer and Renderpass to BeginCommandBuffer()";
    if (!strstr(msgString.c_str(),"may not specify framebuffer or renderpass parameters")) {
        FAIL() << "Error received was not 'vkCreateCommandBuffer():  Primary Command Buffer may not specify framebuffer or renderpass parameters'";
    }
}

TEST_F(VkLayerTest, SecondaryCmdBufferFramebufferAndRenderpass)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;
    VkCmdBuffer     draw_cmd;
    VkCmdPool       cmd_pool;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    VkCmdBufferCreateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmd.pNext = NULL;
    cmd.cmdPool = m_cmdPool;
    cmd.level = VK_CMD_BUFFER_LEVEL_SECONDARY;
    cmd.flags = 0;

    err = vkCreateCommandBuffer(m_device->device(), &cmd, &draw_cmd);
    assert(!err);

    // Force the failure by not setting the Renderpass and Framebuffer fields
    VkCmdBufferBeginInfo cmd_buf_info = {};
    cmd_buf_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_buf_info.pNext = NULL;
    cmd_buf_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                         VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;

    // The error should be caught by validation of the BeginCommandBuffer call
    vkBeginCommandBuffer(draw_cmd, &cmd_buf_info);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error passing NULL Framebuffer/Renderpass to BeginCommandBuffer()";
    if (!strstr(msgString.c_str(),"must specify framebuffer and renderpass parameters")) {
        FAIL() << "Error received was not 'vkCreateCommandBuffer():  Secondary Command Buffer must specify framebuffer and renderpass parameters'";
    }
}

TEST_F(VkLayerTest, InvalidPipelineCreateState)
{
    // Attempt to Create Gfx Pipeline w/o a VS
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;
 
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkGraphicsPipelineCreateInfo gp_ci = {};
        gp_ci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        gp_ci.pNext = NULL;
        gp_ci.stageCount = 0;
        gp_ci.pStages = NULL;
        gp_ci.pVertexInputState = NULL;
        gp_ci.pInputAssemblyState = NULL;
        gp_ci.pTessellationState = NULL;
        gp_ci.pViewportState = NULL;
        gp_ci.pRasterState = NULL;
        gp_ci.pMultisampleState = NULL;
        gp_ci.pDepthStencilState = NULL;
        gp_ci.pColorBlendState = NULL;
        gp_ci.flags = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT;
        gp_ci.layout = pipeline_layout;

    VkPipelineCacheCreateInfo pc_ci = {};
        pc_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
        pc_ci.pNext = NULL;
        pc_ci.initialSize = 0;
        pc_ci.initialData = 0;
        pc_ci.maxSize = 0;

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after creating Gfx Pipeline w/o VS.";
    if (!strstr(msgString.c_str(),"Invalid Pipeline CreateInfo State: Vtx Shader required")) {
        FAIL() << "Error received was not 'Invalid Pipeline CreateInfo State: Vtx Shader required'";
    }
}

TEST_F(VkLayerTest, NullRenderPass)
{
    // Bind a NULL RenderPass
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();

    BeginCommandBuffer();
    // Don't care about RenderPass handle b/c error should be flagged before that
    vkCmdBeginRenderPass(m_cmdBuffer->GetBufferHandle(), NULL, VK_RENDER_PASS_CONTENTS_INLINE);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding NULL RenderPass.";
    if (!strstr(msgString.c_str(),"You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()")) {
        FAIL() << "Error received was not 'You cannot use a NULL RenderPass object in vkCmdBeginRenderPass()'";
    }
}

TEST_F(VkLayerTest, RenderPassWithinRenderPass)
{
    // Bind a BeginRenderPass within an active RenderPass
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();

    BeginCommandBuffer();
    // Just create a dummy Renderpass that's non-NULL so we can get to the proper error
    VkRenderPassBeginInfo rp_begin = {};
        rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rp_begin.pNext = NULL;
        rp_begin.renderPass = (VkRenderPass)0xc001d00d;
        rp_begin.framebuffer = 0;
 
    vkCmdBeginRenderPass(m_cmdBuffer->GetBufferHandle(), &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding RenderPass w/i an active RenderPass.";
    if (!strstr(msgString.c_str(),"Cannot call vkCmdBeginRenderPass() during an active RenderPass ")) {
        FAIL() << "Error received was not 'Cannot call vkCmdBeginRenderPass() during an active RenderPass...'";
    }
}

TEST_F(VkLayerTest, InvalidDynamicStateObject)
{
    // Create a valid cmd buffer
    // call vkCmdBindDynamicStateObject w/ false DS Obj
    // TODO : Simple check for bad object should be added to ObjectTracker to catch this case
    //   The DS check for this is after driver has been called to validate DS internal data struct
}

TEST_F(VkLayerTest, VtxBufferNoRenderPass)
{
    // Bind VBO out-of-bounds for given PSO
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT, this); //  TODO - We shouldn't need a fragment shader
                                                                                       // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    ASSERT_VK_SUCCESS(err);
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // Should error before calling to driver so don't care about actual data
    vkCmdBindVertexBuffers(m_cmdBuffer->GetBufferHandle(), 0, 1, NULL, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after vkCmdBindVertexBuffers() w/o active RenderPass.";
    if (!strstr(msgString.c_str(),"Incorrect call to vkCmdBindVertexBuffers() without an active RenderPass.")) {
        FAIL() << "Error received was not 'Incorrect call to vkCmdBindVertexBuffers() without an active RenderPass.'";
    }
}

TEST_F(VkLayerTest, DSTypeMismatch)
{
    // Create DS w/ layout of one type and attempt Update w/ mis-matched type
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    //VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.pNext = NULL;
        sampler_ci.magFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.minFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.mipMode = VK_TEX_MIPMAP_MODE_BASE;
        sampler_ci.addressU = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressV = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressW = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.mipLodBias = 1.0;
        sampler_ci.maxAnisotropy = 1;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
        sampler_ci.minLod = 1.0;
        sampler_ci.maxLod = 1.0;
        sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(descriptor_info));
    descriptor_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.destSet = descriptorSet;
    descriptor_write.count = 1;
    // This is a mismatched type for the layout which expects BUFFER
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pDescriptors = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after updating BUFFER Descriptor w/ incorrect type of SAMPLER.";
    if (!strstr(msgString.c_str(),"Descriptor update type of VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET does not match ")) {
        FAIL() << "Error received was not 'Descriptor update type of VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET does not match overlapping binding type!'";
    }
}

TEST_F(VkLayerTest, DSUpdateOutOfBounds)
{
    // For overlapping Update, have arrayIndex exceed that of layout
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    //VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.pNext = NULL;
        sampler_ci.magFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.minFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.mipMode = VK_TEX_MIPMAP_MODE_BASE;
        sampler_ci.addressU = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressV = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressW = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.mipLodBias = 1.0;
        sampler_ci.maxAnisotropy = 1;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
        sampler_ci.minLod = 1.0;
        sampler_ci.maxLod = 1.0;
        sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(descriptor_info));
    descriptor_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.destSet = descriptorSet;
    descriptor_write.destArrayElement = 1; /* This index out of bounds for the update */
    descriptor_write.count = 1;
    // This is the wrong type, but out of bounds will be flagged first
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pDescriptors = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after updating Descriptor w/ index out of bounds.";
    if (!strstr(msgString.c_str(),"Descriptor update type of VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET is out of bounds for matching binding")) {
        FAIL() << "Error received was not 'Descriptor update type of VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET is out of bounds for matching binding...'";
    }
}

TEST_F(VkLayerTest, InvalidDSUpdateIndex)
{
    // Create layout w/ count of 1 and attempt update to that layout w/ binding index 2
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    //VkDescriptorSetObj descriptorSet(m_device);
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.pNext = NULL;
        sampler_ci.magFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.minFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.mipMode = VK_TEX_MIPMAP_MODE_BASE;
        sampler_ci.addressU = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressV = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressW = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.mipLodBias = 1.0;
        sampler_ci.maxAnisotropy = 1;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
        sampler_ci.minLod = 1.0;
        sampler_ci.maxLod = 1.0;
        sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;

    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, &sampler);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(descriptor_info));
    descriptor_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptor_write.destSet = descriptorSet;
    descriptor_write.destBinding = 2;
    descriptor_write.count = 1;
    // This is the wrong type, but out of bounds will be flagged first
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pDescriptors = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after updating Descriptor w/ count too large for layout.";
    if (!strstr(msgString.c_str()," does not have binding to match update binding ")) {
        FAIL() << "Error received was not 'Descriptor Set <blah> does not have binding to match update binding '";
    }
}

TEST_F(VkLayerTest, InvalidDSUpdateStruct)
{
    // Call UpdateDS w/ struct type other than valid VK_STRUCTUR_TYPE_UPDATE_* types
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    //VkDescriptorSetObj descriptorSet(m_device);
 
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkSamplerCreateInfo sampler_ci = {};
        sampler_ci.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        sampler_ci.pNext = NULL;
        sampler_ci.magFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.minFilter = VK_TEX_FILTER_NEAREST;
        sampler_ci.mipMode = VK_TEX_MIPMAP_MODE_BASE;
        sampler_ci.addressU = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressV = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.addressW = VK_TEX_ADDRESS_CLAMP;
        sampler_ci.mipLodBias = 1.0;
        sampler_ci.maxAnisotropy = 1;
        sampler_ci.compareEnable = VK_FALSE;
        sampler_ci.compareOp = VK_COMPARE_OP_NEVER;
        sampler_ci.minLod = 1.0;
        sampler_ci.maxLod = 1.0;
        sampler_ci.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
        sampler_ci.unnormalizedCoordinates = VK_FALSE;
    VkSampler sampler;
    err = vkCreateSampler(m_device->device(), &sampler_ci, &sampler);
    ASSERT_VK_SUCCESS(err);


    VkDescriptorInfo descriptor_info;
    memset(&descriptor_info, 0, sizeof(descriptor_info));
    descriptor_info.sampler = sampler;

    VkWriteDescriptorSet descriptor_write;
    memset(&descriptor_write, 0, sizeof(descriptor_write));
    descriptor_write.sType = (VkStructureType)0x99999999; /* Intentionally broken struct type */
    descriptor_write.destSet = descriptorSet;
    descriptor_write.count = 1;
    // This is the wrong type, but out of bounds will be flagged first
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    descriptor_write.pDescriptors = &descriptor_info;

    vkUpdateDescriptorSets(m_device->device(), 1, &descriptor_write, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after updating Descriptor w/ invalid struct type.";
    if (!strstr(msgString.c_str(),"Unexpected UPDATE struct of type ")) {
        FAIL() << "Error received was not 'Unexpected UPDATE struct of type '";
    }
}

TEST_F(VkLayerTest, NumSamplesMismatch)
{
    // Create CmdBuffer where MSAA samples doesn't match RenderPass sampleCount
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();
    VkDescriptorTypeCount ds_type_count = {};
    ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
    ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    ds_pool_ci.pNext = NULL;
    ds_pool_ci.count = 1;
    ds_pool_ci.pTypeCount = &ds_type_count;
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
    dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsl_binding.arraySize = 1;
    dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
    dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
    ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    ds_layout_ci.pNext = NULL;
    ds_layout_ci.count = 1;
    ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
        pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipe_ms_state_ci.pNext = NULL;
        pipe_ms_state_ci.rasterSamples = 4;
        pipe_ms_state_ci.sampleShadingEnable = 0;
        pipe_ms_state_ci.minSampleShading = 1.0;
        pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT, this); //  TODO - We shouldn't need a fragment shader
                                                                                       // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding RenderPass w/ mismatched MSAA from PSO.";
    if (!strstr(msgString.c_str(),"Num samples mismatch! ")) {
        FAIL() << "Error received was not 'Num samples mismatch!...'";
    }
}

TEST_F(VkLayerTest, PipelineNotBound)
{
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();
  
    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;
  
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);
    
    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;

    BeginCommandBuffer();
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding invalid pipeline to CmdBuffer";
    if (!strstr(msgString.c_str(),"Attempt to bind Pipeline ")) {
        FAIL() << "Error received was not 'Attempt to bind Pipeline 0xbaadb1be that doesn't exist!'";
    }
}

TEST_F(VkLayerTest, ClearCmdNoDraw)
{
    // Create CmdBuffer where we add ClearCmd for FB Color attachment prior to issuing a Draw
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();

    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;
  
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
        pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipe_ms_state_ci.pNext = NULL;
        pipe_ms_state_ci.rasterSamples = 4;
        pipe_ms_state_ci.sampleShadingEnable = 0;
        pipe_ms_state_ci.minSampleShading = 1.0;
        pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);
    
    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT, this); //  TODO - We shouldn't need a fragment shader
                                                                                       // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();

    m_errorMonitor->ClearState();
    // Main thing we care about for this test is that the VkImage obj we're clearing matches Color Attachment of FB
    //  Also pass down other dummy params to keep driver and paramchecker happy
    VkClearColorValue cCV;
    cCV.float32[0] = 1.0;
    cCV.float32[1] = 1.0;
    cCV.float32[2] = 1.0;
    cCV.float32[3] = 1.0;

    vkCmdClearColorAttachment(m_cmdBuffer->GetBufferHandle(), 0, (VkImageLayout)NULL, &cCV, 0, NULL);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT) << "Did not receive error after issuing Clear Cmd on FB color attachment prior to Draw Cmd.";
    if (!strstr(msgString.c_str(),"vkCmdClearColorAttachment() issued on CB object ")) {
        FAIL() << "Error received was not 'vkCmdClearColorAttachment() issued on CB object...'";
    }
}

TEST_F(VkLayerTest, VtxBufferBadIndex)
{
    // Create CmdBuffer where MSAA samples doesn't match RenderPass sampleCount
    VkFlags         msgFlags;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    m_errorMonitor->ClearState();

    VkDescriptorTypeCount ds_type_count = {};
        ds_type_count.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        ds_type_count.count = 1;

    VkDescriptorPoolCreateInfo ds_pool_ci = {};
        ds_pool_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        ds_pool_ci.pNext = NULL;
        ds_pool_ci.count = 1;
        ds_pool_ci.pTypeCount = &ds_type_count;

        VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSetLayoutBinding dsl_binding = {};
        dsl_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        dsl_binding.arraySize = 1;
        dsl_binding.stageFlags = VK_SHADER_STAGE_ALL;
        dsl_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo ds_layout_ci = {};
        ds_layout_ci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        ds_layout_ci.pNext = NULL;
        ds_layout_ci.count = 1;
        ds_layout_ci.pBinding = &dsl_binding;

    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet);
    ASSERT_VK_SUCCESS(err);

    VkPipelineMultisampleStateCreateInfo pipe_ms_state_ci = {};
        pipe_ms_state_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        pipe_ms_state_ci.pNext = NULL;
        pipe_ms_state_ci.rasterSamples = 1;
        pipe_ms_state_ci.sampleShadingEnable = 0;
        pipe_ms_state_ci.minSampleShading = 1.0;
        pipe_ms_state_ci.pSampleMask = NULL;

    VkPipelineLayoutCreateInfo pipeline_layout_ci = {};
        pipeline_layout_ci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipeline_layout_ci.pNext = NULL;
        pipeline_layout_ci.descriptorSetCount = 1;
        pipeline_layout_ci.pSetLayouts = &ds_layout;
        VkPipelineLayout pipeline_layout;

    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, bindStateFragShaderText, VK_SHADER_STAGE_FRAGMENT, this); //  TODO - We shouldn't need a fragment shader
                                                                                       // but add it to be able to run on more devices
    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);
    pipe.SetMSAA(&pipe_ms_state_ci);
    pipe.CreateVKPipeline(pipeline_layout, renderPass());

    BeginCommandBuffer();
    vkCmdBindPipeline(m_cmdBuffer->GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipe.handle());
    // Don't care about actual data, just need to get to draw to flag error
    static const float vbo_data[3] = {1.f, 0.f, 1.f};
    VkConstantBufferObj vbo(m_device, sizeof(vbo_data), sizeof(float), (const void*) &vbo_data);
    BindVertexBuffer(&vbo, (VkDeviceSize)0, 1); // VBO idx 1, but no VBO in PSO
    Draw(0, 1, 0, 0);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive error after binding Vtx Buffer w/o VBO attached to PSO.";
    if (!strstr(msgString.c_str(),"Vtx Buffer Index 1 was bound, but no vtx buffers are attached to PSO.")) {
        FAIL() << "Error received was not 'Vtx Buffer Index 0 was bound, but no vtx buffers are attached to PSO.'";
    }
}
#endif
#if THREADING_TESTS
#if GTEST_IS_THREADSAFE
struct thread_data_struct {
    VkCmdBuffer cmdBuffer;
    VkEvent event;
    bool bailout;
};

extern "C" void *AddToCommandBuffer(void *arg)
{
    struct thread_data_struct *data = (struct thread_data_struct *) arg;
    std::string msgString;

    for (int i = 0; i<10000; i++) {
        vkCmdSetEvent(data->cmdBuffer, data->event, VK_PIPELINE_STAGE_ALL_GPU_COMMANDS);
        if (data->bailout) {
            break;
        }
    }
    return NULL;
}

TEST_F(VkLayerTest, ThreadCmdBufferCollision)
{
    VkFlags msgFlags;
    std::string msgString;
    test_platform_thread thread;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    m_errorMonitor->ClearState();
    BeginCommandBuffer();

    VkEventCreateInfo event_info;
    VkEvent event;
    VkResult err;

    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(device(), &event_info, &event);
    ASSERT_VK_SUCCESS(err);

    err = vkResetEvent(device(), event);
    ASSERT_VK_SUCCESS(err);

    struct thread_data_struct data;
    data.cmdBuffer = m_cmdBuffer->handle();
    data.event = event;
    data.bailout = false;
    m_errorMonitor->SetBailout(&data.bailout);
    // Add many entries to command buffer from another thread.
    test_platform_thread_create(&thread, AddToCommandBuffer, (void *)&data);
    // Add many entries to command buffer from this thread at the same time.
    AddToCommandBuffer(&data);
    test_platform_thread_join(thread, NULL);
    EndCommandBuffer();

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT)) << "Did not receive an err from using one VkCommandBufferObj in two threads";
    if (!strstr(msgString.c_str(),"THREADING ERROR")) {
        FAIL() << "Error received was not 'THREADING ERROR'";
    }

}
#endif
#endif
#if SHADER_CHECKER_TESTS
TEST_F(VkLayerTest, CreatePipelineVertexOutputNotConsumed)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out float x;\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "   x = 0;\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT);
    if (!strstr(msgString.c_str(),"not consumed by fragment shader")) {
        FAIL() << "Incorrect warning: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineFragmentInputNotProvided)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) in float x;\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(x);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"not written by vertex shader")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineVsFsTypeMismatch)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out int x;\n"
        "void main(){\n"
        "   x = 0;\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) in float x;\n"  /* VS writes int */
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(x);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"Type mismatch on location 0")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineAttribNotConsumed)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT);
    if (!strstr(msgString.c_str(),"location 0 not consumed by VS")) {
        FAIL() << "Incorrect warning: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineAttribNotProvided)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) in vec4 x;\n"       /* not provided */
        "void main(){\n"
        "   gl_Position = x;\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"VS consumes input at location 0 but not provided")) {
        FAIL() << "Incorrect warning: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineAttribTypeMismatch)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    VkVertexInputBindingDescription input_binding;
    memset(&input_binding, 0, sizeof(input_binding));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) in int x;\n"    /* attrib provided float */
        "void main(){\n"
        "   gl_Position = vec4(x);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(&input_binding, 1);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"location 0 does not match VS input type")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineAttribBindingConflict)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    /* Two binding descriptions for binding 0 */
    VkVertexInputBindingDescription input_bindings[2];
    memset(input_bindings, 0, sizeof(input_bindings));

    VkVertexInputAttributeDescription input_attrib;
    memset(&input_attrib, 0, sizeof(input_attrib));
    input_attrib.format = VK_FORMAT_R32_SFLOAT;

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) in float x;\n"    /* attrib provided float */
        "void main(){\n"
        "   gl_Position = vec4(x);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 color;\n"
        "void main(){\n"
        "   color = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddColorAttachment();
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    pipe.AddVertexInputBindings(input_bindings, 2);
    pipe.AddVertexInputAttribs(&input_attrib, 1);

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"Duplicate vertex input binding descriptions for binding 0")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

/* TODO: would be nice to test the mixed broadcast & custom case, but the GLSL->SPV compiler
 * rejects it. */

TEST_F(VkLayerTest, CreatePipelineFragmentOutputNotWritten)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0, not written */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"Attachment 0 not written by FS")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineFragmentOutputNotConsumed)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 x;\n"
        "layout(location=1) out vec4 y;\n"  /* no matching attachment for this */
        "void main(){\n"
        "   x = vec4(1);\n"
        "   y = vec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0, not written */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    /* FS writes CB 1, but we don't configure it */

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT);
    if (!strstr(msgString.c_str(),"FS writes to output location 1 with no matching attachment")) {
        FAIL() << "Incorrect warning: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineFragmentOutputTypeMismatch)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    ScopedUseGlsl useGlsl(false);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out ivec4 x;\n"     /* not UNORM */
        "void main(){\n"
        "   x = ivec4(1);\n"
        "}\n";

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);

    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0; type is UNORM by default */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(0 != (msgFlags & VK_DBG_REPORT_ERROR_BIT));
    if (!strstr(msgString.c_str(),"does not match FS output type")) {
        FAIL() << "Incorrect error: " << msgString;
    }
}

TEST_F(VkLayerTest, CreatePipelineNonSpirvShader)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    /* Intentionally provided GLSL rather than compiling to SPIRV first */
    ScopedUseGlsl useGlsl(true);

    char const *vsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "void main(){\n"
        "   gl_Position = vec4(1);\n"
        "}\n";
    char const *fsSource =
        "#version 140\n"
        "#extension GL_ARB_separate_shader_objects: require\n"
        "#extension GL_ARB_shading_language_420pack: require\n"
        "\n"
        "layout(location=0) out vec4 x;\n"
        "void main(){\n"
        "   x = vec4(1);\n"
        "}\n";

    m_errorMonitor->ClearState();

    VkShaderObj vs(m_device, vsSource, VK_SHADER_STAGE_VERTEX, this);
    VkShaderObj fs(m_device, fsSource, VK_SHADER_STAGE_FRAGMENT, this);


    VkPipelineObj pipe(m_device);
    pipe.AddShader(&vs);
    pipe.AddShader(&fs);

    /* set up CB 0; type is UNORM by default */
    pipe.AddColorAttachment();
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(m_cmdBuffer);

    VkResult res = pipe.CreateVKPipeline(descriptorSet.GetPipelineLayout(), renderPass());
    /* pipeline creation should have succeeded */
    ASSERT_EQ(VK_SUCCESS, res);

    /* should have emitted a warning: the shader is not SPIRV, so we're
     * not going to be able to analyze it */
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_NE(0, msgFlags & VK_DBG_REPORT_WARN_BIT);
    if (!strstr(msgString.c_str(),"is not SPIR-V")) {
        FAIL() << "Incorrect warning: " << msgString;
    }
}
#endif

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);
    VkTestFramework::InitArgs(&argc, argv);

    ::testing::AddGlobalTestEnvironment(new TestEnvironment);

    result = RUN_ALL_TESTS();

    VkTestFramework::Finish();
    return result;
}

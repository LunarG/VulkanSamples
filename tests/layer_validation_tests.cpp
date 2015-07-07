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
    BsoFailRaster                   = 0x00000001,
    BsoFailViewport                 = 0x00000002,
    BsoFailColorBlend               = 0x00000004,
    BsoFailDepthStencil             = 0x00000008,
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
    VkFlags                    msgFlags,
    VkObjectType                        objType,
    VkObject                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData);

class ErrorMonitor {
public:
    ErrorMonitor()
    {
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutex_lock(&m_mutex);
        m_msgFlags = VK_DBG_REPORT_INFO_BIT;
        m_bailout = NULL;
        pthread_mutex_unlock(&m_mutex);
    }
    void ClearState()
    {
        pthread_mutex_lock(&m_mutex);
        m_msgFlags = VK_DBG_REPORT_INFO_BIT;
        m_msgString.clear();
        pthread_mutex_unlock(&m_mutex);
    }
    VkFlags GetState(std::string *msgString)
    {
        pthread_mutex_lock(&m_mutex);
        *msgString = m_msgString;
        pthread_mutex_unlock(&m_mutex);
        return m_msgFlags;
    }
    void SetState(VkFlags msgFlags, const char *msgString)
    {
        pthread_mutex_lock(&m_mutex);
        if (m_bailout != NULL) {
            *m_bailout = true;
        }
        m_msgFlags = msgFlags;
        m_msgString.reserve(strlen(msgString));
        m_msgString = msgString;
        pthread_mutex_unlock(&m_mutex);
    }
    void SetBailout(bool *bailout)
    {
        m_bailout = bailout;
    }

private:
    VkFlags                m_msgFlags;
    std::string            m_msgString;
    pthread_mutex_t        m_mutex;
    bool*                  m_bailout;
};

static void myDbgFunc(
    VkFlags                    msgFlags,
    VkObjectType               objType,
    VkObject                   srcObject,
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

protected:
        VkMemoryRefManager         m_memoryRefManager;
        ErrorMonitor               *m_errorMonitor;

    virtual void SetUp() {
        std::vector<const char *> instance_layer_names;
        std::vector<const char *> device_layer_names;
        std::vector<const char *> instance_extension_names;
        std::vector<const char *> device_extension_names;

        instance_extension_names.push_back(DEBUG_REPORT_EXTENSION_NAME);
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
    VkCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    ASSERT_VK_SUCCESS(BeginCommandBuffer(cmdBuffer));

    GenericDrawPreparation(&cmdBuffer, pipelineobj, descriptorSet, failMask);

    // render triangle
    cmdBuffer.Draw(0, 3, 0, 1);

    // finalize recording of the command buffer
    EndCommandBuffer(cmdBuffer);

    cmdBuffer.QueueCommandBuffer();
}

void VkLayerTest::GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask)
{
    if (m_depthStencil->Initialized()) {
        cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, m_depthStencil);
    } else {
        cmdBuffer->ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    }

    cmdBuffer->PrepareAttachments();
    if ((failMask & BsoFailRaster) != BsoFailRaster) {
        cmdBuffer->BindStateObject(VK_STATE_BIND_POINT_RASTER, m_stateRaster);
    }
    if ((failMask & BsoFailViewport) != BsoFailViewport) {
        cmdBuffer->BindStateObject(VK_STATE_BIND_POINT_VIEWPORT, m_stateViewport);
    }
    if ((failMask & BsoFailColorBlend) != BsoFailColorBlend) {
        cmdBuffer->BindStateObject(VK_STATE_BIND_POINT_COLOR_BLEND, m_colorBlend);
    }
    if ((failMask & BsoFailDepthStencil) != BsoFailDepthStencil) {
        cmdBuffer->BindStateObject(VK_STATE_BIND_POINT_DEPTH_STENCIL, m_stateDepthStencil);
    }
    // Make sure depthWriteEnable is set so that DepthStencil fail test will work correctly
    VkStencilOpState stencil = {
        .stencilFailOp = VK_STENCIL_OP_KEEP,
        .stencilPassOp = VK_STENCIL_OP_KEEP,
        .stencilDepthFailOp = VK_STENCIL_OP_KEEP,
        .stencilCompareOp = VK_COMPARE_OP_NEVER
    };
    VkPipelineDsStateCreateInfo ds_ci = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DS_STATE_CREATE_INFO,
        .pNext = NULL,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_NEVER,
        .depthBoundsEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = stencil,
        .back = stencil
    };
    pipelineobj.SetDepthStencil(&ds_ci);
    descriptorSet.CreateVKDescriptorSet(cmdBuffer);
    pipelineobj.CreateVKPipeline(descriptorSet, renderPass());
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
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    BeginCommandBuffer(cmdBuffer);
    cmdBuffer.ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer(cmdBuffer);

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    err = vkQueueSubmit( m_device->m_queue, 1, &cmdBuffer.obj(), testFence.obj());
    ASSERT_VK_SUCCESS( err );

    m_errorMonitor->ClearState();
    // Introduce failure by calling begin again before checking fence
    vkResetCommandBuffer(cmdBuffer.obj());

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an err after calling ResetCommandBuffer on an active Command Buffer";
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

    VkCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    BeginCommandBuffer(cmdBuffer);
    cmdBuffer.ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer(cmdBuffer);

    testFence.init(*m_device, fenceInfo);

    // Bypass framework since it does the waits automatically
    VkResult err = VK_SUCCESS;
    err = vkQueueSubmit( m_device->m_queue, 1, &cmdBuffer.obj(), testFence.obj());
    ASSERT_VK_SUCCESS( err );

    m_errorMonitor->ClearState();
    // Introduce failure by calling begin again before checking fence
    BeginCommandBuffer(cmdBuffer);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an err after calling BeginCommandBuffer on an active Command Buffer";
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

    const VkImageCreateInfo image_create_info = {
        .sType          = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext          = NULL,
        .imageType      = VK_IMAGE_TYPE_2D,
        .format         = tex_format,
        .extent         = { tex_width, tex_height, 1 },
        .mipLevels      = 1,
        .arraySize      = 1,
        .samples        = 1,
        .tiling         = VK_IMAGE_TILING_LINEAR,
        .usage          = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags          = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType          = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext          = NULL,
        .allocationSize = 0,
        // Introduce failure, do NOT set memProps to VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
        .memoryTypeIndex = 1,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // Map memory as if to initialize the image
    void *mappedAddress = NULL;
    err = vkMapMemory(m_device->device(), mem, 0, 0, 0, &mappedAddress);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error while tring to map memory not visible to CPU";
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

    const VkImageCreateInfo image_create_info = {
        .sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext           = NULL,
        .imageType       = VK_IMAGE_TYPE_2D,
        .format          = tex_format,
        .extent          = { tex_width, tex_height, 1 },
        .mipLevels       = 1,
        .arraySize       = 1,
        .samples         = 1,
        .tiling          = VK_IMAGE_TILING_LINEAR,
        .usage           = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags           = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext           = NULL,
        .allocationSize  = 0,
        .memoryTypeIndex = 0,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;

    err = m_device->gpu().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory before binding
    vkFreeMemory(m_device->device(), mem);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error while tring to bind a freed memory object";
    if (!strstr(msgString.c_str(),"couldn't find info for mem obj")) {
        FAIL() << "Error received did not match expected error message from BindObjectMemory in MemTracker";
    }
}

TEST_F(VkLayerTest, FreeBoundMemory)
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

    const VkImageCreateInfo image_create_info = {
        .sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext           = NULL,
        .imageType       = VK_IMAGE_TYPE_2D,
        .format          = tex_format,
        .extent          = { tex_width, tex_height, 1 },
        .mipLevels       = 1,
        .arraySize       = 1,
        .samples         = 1,
        .tiling          = VK_IMAGE_TILING_LINEAR,
        .usage           = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags           = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext           = NULL,
        .allocationSize  = 0,
        .memoryTypeIndex = 0,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;

    err = m_device->gpu().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Bind memory to Image object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory while still bound to object
    vkFreeMemory(m_device->device(), mem);
    ASSERT_VK_SUCCESS(err);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an warning while tring to free bound memory";
    if (!strstr(msgString.c_str(),"Freeing memory object while it still has references")) {
        FAIL() << "Warning received did not match expected message from freeMemObjInfo  in MemTracker";
    }
}

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

    const VkImageCreateInfo image_create_info = {
        .sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext           = NULL,
        .imageType       = VK_IMAGE_TYPE_2D,
        .format          = tex_format,
        .extent          = { tex_width, tex_height, 1 },
        .mipLevels       = 1,
        .arraySize       = 1,
        .samples         = 1,
        .tiling          = VK_IMAGE_TILING_LINEAR,
        .usage           = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags           = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext           = NULL,
        .allocationSize  = 0,
        .memoryTypeIndex = 0,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;
    err = m_device->gpu().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // allocate 2 memory objects
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem1);
    ASSERT_VK_SUCCESS(err);
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem2);
    ASSERT_VK_SUCCESS(err);

    // Bind first memory object to Image object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem1, 0);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, try to bind a different memory object to the same image object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem2, 0);
    ASSERT_VK_SUCCESS(err);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error while tring to rebind an object";
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

    const VkImageCreateInfo image_create_info = {
        .sType           = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .pNext           = NULL,
        .imageType       = VK_IMAGE_TYPE_2D,
        .format          = tex_format,
        .extent          = { tex_width, tex_height, 1 },
        .mipLevels       = 1,
        .arraySize       = 1,
        .samples         = 1,
        .tiling          = VK_IMAGE_TILING_LINEAR,
        .usage           = VK_IMAGE_USAGE_SAMPLED_BIT,
        .flags           = 0,
    };
    VkMemoryAllocInfo mem_alloc = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO,
        .pNext           = NULL,
        .allocationSize  = 0,
        .memoryTypeIndex = 0,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          &mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs.size;
    err = m_device->gpu().set_memory_type(mem_reqs.memoryTypeBits, &mem_alloc, 0);
    ASSERT_VK_SUCCESS(err);

    // Allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &mem);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, destroy Image object before binding
    vkDestroyObject(m_device->device(), VK_OBJECT_TYPE_IMAGE, image);
    ASSERT_VK_SUCCESS(err);

    // Now Try to bind memory to this destroyted object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, mem, 0);
    ASSERT_VK_SUCCESS(err);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error while binding memory to a destroyed object";
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

    VkCommandBufferObj cmdBuffer(m_device);
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);

    BeginCommandBuffer(cmdBuffer);
    cmdBuffer.ClearAllBuffers(m_clear_color, m_depth_clear_color, m_stencil_clear_color, NULL);
    EndCommandBuffer(cmdBuffer);

    testFence.init(*m_device, fenceInfo);
    m_errorMonitor->ClearState();
    cmdBuffer.QueueCommandBuffer(testFence.obj());
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an err from using a fence in SIGNALED state in call to vkQueueSubmit";
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
    VkFence fences[1] = {testFence.obj()};
    vkResetFences(m_device->device(), 1, fences);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from submitting fence with UNSIGNALED state to vkResetFences";
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
    BeginCommandBuffer(cmdBuffer);

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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after attempting to create DSView w/ image lacking USAGE_DS_BIT flag";
    if (!strstr(msgString.c_str(),"Invalid usage flag for image ")) {
        FAIL() << "Error received was not 'Invalid usage flag for image...'";
    }
}
#endif
#endif
#if OBJ_TRACKER_TESTS
TEST_F(VkLayerTest, RasterStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a raster state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailRaster);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from Not Binding a Raster State Object";
    if (!strstr(msgString.c_str(),"Raster object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Raster object not bound to this command buffer'";
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from Not Binding a Viewport State Object";
    if (!strstr(msgString.c_str(),"Viewport object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Viewport object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, ColorBlendStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a color-blend state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailColorBlend);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from Not Binding a ColorBlend State Object";
    if (!strstr(msgString.c_str(),"Color-blend object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Color-blend object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, DepthStencilStateNotBound)
{
    VkFlags msgFlags;
    std::string msgString;
    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a depth-stencil state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthStencil);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from Not Binding a DepthStencil State Object";
    if (!strstr(msgString.c_str(),"Depth-stencil object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Depth-stencil object not bound to this command buffer'";
    }
}
#endif
#if DRAW_STATE_TESTS
TEST_F(VkLayerTest, BindPipelineNoRenderPass)
{
    // Initiate Draw w/o a PSO bound
    VkFlags         msgFlags;
    std::string     msgString;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();
    VkCommandBufferObj cmdBuffer(m_device);
    BeginCommandBuffer(cmdBuffer);
    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding pipeline to CmdBuffer w/o active RenderPass";
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an error from Resetting an invalid DescriptorPool Object";
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
//    BeginCommandBuffer(cmdBuffer);
//    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;
//    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);
//    msgFlags = m_errorMonitor->GetState(&msgString);
//    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding invalid pipeline to CmdBuffer";
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
    m_errorMonitor->ClearState();
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext               = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);

    const VkPipelineShaderStageCreateInfo pipe_vs_ci = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                = NULL,
        .stage                = VK_SHADER_STAGE_VERTEX,
        .shader               = vs.obj(),
        .linkConstBufferCount = 0,
        .pLinkConstBufferInfo = NULL,
        .pSpecializationInfo  = NULL,
    };
    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 1,
        .pStages           = &pipe_vs_ci,
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = NULL,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };
    const VkPipelineCacheCreateInfo pc_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext             = NULL,
        .initialSize       = 0,
        .initialData       = 0,
        .maxSize           = 0,
    };

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);
    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());
    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout, 0, 1, &descriptorSet, 0, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT) << "Did not warn after binding a DescriptorSet that was never updated.";
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
    VkCommandBufferObj cmdBuffer(m_device);
    // Call EndCommandBuffer() w/o calling BeginCommandBuffer()
    vkEndCommandBuffer(cmdBuffer.GetBufferHandle());
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after ending a CmdBuffer w/o calling BeginCommandBuffer()";
    if (!strstr(msgString.c_str(),"You must call vkBeginCommandBuffer() before this call to ")) {
        FAIL() << "Error received was not 'You must call vkBeginCommandBuffer() before this call to vkEndCommandBuffer()'";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext               = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 0,
        .pStages           = NULL, // Creating Gfx Pipeline w/o VS is a violation
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = NULL,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };
    const VkPipelineCacheCreateInfo pc_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext             = NULL,
        .initialSize       = 0,
        .initialData       = 0,
        .maxSize           = 0,
    };

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after creating Gfx Pipeline w/o VS.";
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
    VkCommandBufferObj cmdBuffer(m_device);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    // Don't care about RenderPass handle b/c error should be flagged before that
    vkCmdBeginRenderPass(cmdBuffer.GetBufferHandle(), NULL, VK_RENDER_PASS_CONTENTS_INLINE);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding NULL RenderPass.";
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
    VkCommandBufferObj cmdBuffer(m_device);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    // Just create a dummy Renderpass that's non-NULL so we can get to the proper error
    const VkRenderPassBeginInfo rp_begin = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .pNext = NULL,
        .renderPass = (VkRenderPass) 0xc001d00d,
        .framebuffer = NULL
    };
    vkCmdBeginRenderPass(cmdBuffer.GetBufferHandle(), &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding RenderPass w/i an active RenderPass.";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext               = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);

    const VkPipelineShaderStageCreateInfo pipe_vs_ci = {
        .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext  = NULL,
        .stage                = VK_SHADER_STAGE_VERTEX,
        .shader               = vs.obj(),
        .linkConstBufferCount = 0,
        .pLinkConstBufferInfo = NULL,
        .pSpecializationInfo  = NULL,
    };
    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 1,
        .pStages           = &pipe_vs_ci,
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = NULL,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };
    const VkPipelineCacheCreateInfo pc_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext             = NULL,
        .initialSize       = 0,
        .initialData       = 0,
        .maxSize           = 0,
    };

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);

    err= cmdBuffer.BeginCommandBuffer();
    ASSERT_VK_SUCCESS(err);
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // Should error before calling to driver so don't care about actual data
    vkCmdBindVertexBuffers(cmdBuffer.GetBufferHandle(), 0, 1, NULL, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after vkCmdBindVertexBuffers() w/o active RenderPass.";
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
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkSamplerCreateInfo sampler_ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .magFilter    = VK_TEX_FILTER_NEAREST,
        .minFilter    = VK_TEX_FILTER_NEAREST,
        .mipMode      = VK_TEX_MIPMAP_MODE_BASE,
        .addressU     = VK_TEX_ADDRESS_CLAMP,
        .addressV     = VK_TEX_ADDRESS_CLAMP,
        .addressW     = VK_TEX_ADDRESS_CLAMP,
        .mipLodBias   = 1.0,
        .maxAnisotropy = 1,
        .compareOp    = VK_COMPARE_OP_NEVER,
        .minLod       = 1.0,
        .maxLod       = 1.0,
        .borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    };
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after updating BUFFER Descriptor w/ incorrect type of SAMPLER.";
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
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkSamplerCreateInfo sampler_ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .magFilter    = VK_TEX_FILTER_NEAREST,
        .minFilter    = VK_TEX_FILTER_NEAREST,
        .mipMode      = VK_TEX_MIPMAP_MODE_BASE,
        .addressU     = VK_TEX_ADDRESS_CLAMP,
        .addressV     = VK_TEX_ADDRESS_CLAMP,
        .addressW     = VK_TEX_ADDRESS_CLAMP,
        .mipLodBias   = 1.0,
        .maxAnisotropy = 1,
        .compareOp    = VK_COMPARE_OP_NEVER,
        .minLod       = 1.0,
        .maxLod       = 1.0,
        .borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    };
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after updating Descriptor w/ index out of bounds.";
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
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkSamplerCreateInfo sampler_ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .magFilter    = VK_TEX_FILTER_NEAREST,
        .minFilter    = VK_TEX_FILTER_NEAREST,
        .mipMode      = VK_TEX_MIPMAP_MODE_BASE,
        .addressU     = VK_TEX_ADDRESS_CLAMP,
        .addressV     = VK_TEX_ADDRESS_CLAMP,
        .addressW     = VK_TEX_ADDRESS_CLAMP,
        .mipLodBias   = 1.0,
        .maxAnisotropy = 1,
        .compareOp    = VK_COMPARE_OP_NEVER,
        .minLod       = 1.0,
        .maxLod       = 1.0,
        .borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    };
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after updating Descriptor w/ count too large for layout.";
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
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);
    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkSamplerCreateInfo sampler_ci = {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .pNext = NULL,
        .magFilter    = VK_TEX_FILTER_NEAREST,
        .minFilter    = VK_TEX_FILTER_NEAREST,
        .mipMode      = VK_TEX_MIPMAP_MODE_BASE,
        .addressU     = VK_TEX_ADDRESS_CLAMP,
        .addressV     = VK_TEX_ADDRESS_CLAMP,
        .addressW     = VK_TEX_ADDRESS_CLAMP,
        .mipLodBias   = 1.0,
        .maxAnisotropy = 1,
        .compareOp    = VK_COMPARE_OP_NEVER,
        .minLod       = 1.0,
        .maxLod       = 1.0,
        .borderColor  = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
    };
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
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after updating Descriptor w/ invalid struct type.";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineMsStateCreateInfo pipe_ms_state_ci = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO,
        .pNext               = NULL,
        .rasterSamples       = 4,
        .multisampleEnable   = 1,
        .sampleShadingEnable = 0,
        .minSampleShading    = 1.0,
        .sampleMask          = 15,
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext              = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);

    const VkPipelineShaderStageCreateInfo pipe_vs_ci = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                = NULL,
        .stage                = VK_SHADER_STAGE_VERTEX,
        .shader               = vs.obj(),
        .linkConstBufferCount = 0,
        .pLinkConstBufferInfo = NULL,
        .pSpecializationInfo  = NULL,
    };
    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 1,
        .pStages           = &pipe_vs_ci,
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = &pipe_ms_state_ci,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };
    const VkPipelineCacheCreateInfo pc_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext             = NULL,
        .initialSize       = 0,
        .initialData       = 0,
        .maxSize           = 0,
    };

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding RenderPass w/ mismatched MSAA from PSO.";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext              = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkPipeline badPipeline = (VkPipeline)0xbaadb1be;
    //err = vkCreateGraphicsPipeline(m_device->device(), &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, badPipeline);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding invalid pipeline to CmdBuffer";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineMsStateCreateInfo pipe_ms_state_ci = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO,
        .pNext               = NULL,
        .rasterSamples       = 4,
        .multisampleEnable   = 1,
        .sampleShadingEnable = 0,
        .minSampleShading    = 1.0,
        .sampleMask          = 15,
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext              = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    size_t shader_len = strlen(bindStateVertShaderText);
    size_t codeSize = 3 * sizeof(uint32_t) + shader_len + 1;
    void* pCode = malloc(codeSize);

    /* try version 0 first: VkShaderStage followed by GLSL */
    ((uint32_t *) pCode)[0] = ICD_SPV_MAGIC;
    ((uint32_t *) pCode)[1] = 0;
    ((uint32_t *) pCode)[2] = VK_SHADER_STAGE_VERTEX;
    memcpy(((uint32_t *) pCode + 3), bindStateVertShaderText, shader_len + 1);

    const VkShaderModuleCreateInfo smci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .codeSize = codeSize,
        .pCode = pCode,
        .flags = 0,
    };
    VkShaderModule vksm;
    err = vkCreateShaderModule(m_device->device(), &smci, &vksm);
    ASSERT_VK_SUCCESS(err);
    const VkShaderCreateInfo vs_ci = {
        .sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO,
        .pNext = NULL,
        .module = vksm,
        .pName = "main",
        .flags = 0,
    };
    VkShader vs;
    err = vkCreateShader(m_device->device(), &vs_ci, &vs);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineShaderStageCreateInfo pipe_vs_ci = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                = NULL,
        .stage                = VK_SHADER_STAGE_VERTEX,
        .shader               = vs,
        .linkConstBufferCount = 0,
        .pLinkConstBufferInfo = NULL,
        .pSpecializationInfo  = NULL,
    };
    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 1,
        .pStages           = &pipe_vs_ci,
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = &pipe_ms_state_ci,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };
    const VkPipelineCacheCreateInfo pc_ci = {
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
        .pNext             = NULL,
        .initialSize       = 0,
        .initialData       = 0,
        .maxSize           = 0,
    };

    VkPipeline pipeline;
    VkPipelineCache pipelineCache;

    err = vkCreatePipelineCache(m_device->device(), &pc_ci, &pipelineCache);
    ASSERT_VK_SUCCESS(err);
    err = vkCreateGraphicsPipelines(m_device->device(), pipelineCache, 1, &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);

    m_errorMonitor->ClearState();
    // Main thing we care about for this test is that the VkImage obj we're clearing matches Color Attachment of FB
    //  Also pass down other dummy params to keep driver and paramchecker happy
    VkClearColorValue cCV;
    cCV.f32[0] = 1.0;
    cCV.f32[1] = 1.0;
    cCV.f32[2] = 1.0;
    cCV.f32[3] = 1.0;

    vkCmdClearColorAttachment(cmdBuffer.GetBufferHandle(), 0, (VkImageLayout)NULL, &cCV, 0, NULL);
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT) << "Did not receive error after issuing Clear Cmd on FB color attachment prior to Draw Cmd.";
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
    VkCommandBufferObj cmdBuffer(m_device);
    const VkDescriptorTypeCount ds_type_count = {
        .type       = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .count      = 1,
    };
    const VkDescriptorPoolCreateInfo ds_pool_ci = {
        .sType      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext      = NULL,
        .count      = 1,
        .pTypeCount = &ds_type_count,
    };
    VkDescriptorPool ds_pool;
    err = vkCreateDescriptorPool(m_device->device(), VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1, &ds_pool_ci, &ds_pool);
    ASSERT_VK_SUCCESS(err);

    const VkDescriptorSetLayoutBinding dsl_binding = {
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .arraySize          = 1,
        .stageFlags         = VK_SHADER_STAGE_ALL,
        .pImmutableSamplers = NULL,
    };

    const VkDescriptorSetLayoutCreateInfo ds_layout_ci = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .count = 1,
        .pBinding = &dsl_binding,
    };
    VkDescriptorSetLayout ds_layout;
    err = vkCreateDescriptorSetLayout(m_device->device(), &ds_layout_ci, &ds_layout);
    ASSERT_VK_SUCCESS(err);

    VkDescriptorSet descriptorSet;
    uint32_t ds_count = 0;
    err = vkAllocDescriptorSets(m_device->device(), ds_pool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, 1, &ds_layout, &descriptorSet, &ds_count);
    ASSERT_VK_SUCCESS(err);

    const VkPipelineMsStateCreateInfo pipe_ms_state_ci = {
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_MS_STATE_CREATE_INFO,
        .pNext               = NULL,
        .rasterSamples       = 1,
        .multisampleEnable   = 1,
        .sampleShadingEnable = 0,
        .minSampleShading    = 1.0,
        .sampleMask          = 15,
    };

    const VkPipelineLayoutCreateInfo pipeline_layout_ci = {
        .sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext              = NULL,
        .descriptorSetCount = 1,
        .pSetLayouts        = &ds_layout,
    };

    VkPipelineLayout pipeline_layout;
    err = vkCreatePipelineLayout(m_device->device(), &pipeline_layout_ci, &pipeline_layout);
    ASSERT_VK_SUCCESS(err);

    VkShaderObj vs(m_device, bindStateVertShaderText, VK_SHADER_STAGE_VERTEX, this);

    const VkPipelineShaderStageCreateInfo pipe_vs_ci = {
        .sType                = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext                = NULL,
        .stage                = VK_SHADER_STAGE_VERTEX,
        .shader               = vs.obj(),
        .linkConstBufferCount = 0,
        .pLinkConstBufferInfo = NULL,
        .pSpecializationInfo  = NULL,
    };
    const VkGraphicsPipelineCreateInfo gp_ci = {
        .sType             = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext             = NULL,
        .stageCount        = 1,
        .pStages           = &pipe_vs_ci,
        .pVertexInputState = NULL,
        .pIaState          = NULL,
        .pTessState        = NULL,
        .pVpState          = NULL,
        .pRsState          = NULL,
        .pMsState          = &pipe_ms_state_ci,
        .pDsState          = NULL,
        .pCbState          = NULL,
        .flags             = VK_PIPELINE_CREATE_DISABLE_OPTIMIZATION_BIT,
        .layout            = pipeline_layout,
    };

    VkPipelineCacheCreateInfo pipelineCache;
    VkPipelineCache pipeline_cache;

    memset(&pipelineCache, 0, sizeof(pipelineCache));
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    err = vkCreatePipelineCache(m_device->device(), &pipelineCache, &pipeline_cache);

    VkPipeline pipeline;
    err = vkCreateGraphicsPipelines(m_device->device(), pipeline_cache, 1, &gp_ci, &pipeline);
    ASSERT_VK_SUCCESS(err);

    cmdBuffer.AddRenderTarget(m_renderTargets[0]);
    BeginCommandBuffer(cmdBuffer);
    vkCmdBindPipeline(cmdBuffer.GetBufferHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    // Should error before calling to driver so don't care about actual data
    vkCmdBindVertexBuffers(cmdBuffer.GetBufferHandle(), 0, 1, NULL, NULL);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive error after binding Vtx Buffer w/o VBO attached to PSO.";
    if (!strstr(msgString.c_str(),"Vtx Buffer Index 0 was bound, but no vtx buffers are attached to PSO.")) {
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
    pthread_t thread;
    pthread_attr_t thread_attr;

    ASSERT_NO_FATAL_FAILURE(InitState());
    ASSERT_NO_FATAL_FAILURE(InitViewport());
    ASSERT_NO_FATAL_FAILURE(InitRenderTarget());

    VkCommandBufferObj cmdBuffer(m_device);

    m_errorMonitor->ClearState();
    pthread_attr_init(&thread_attr);
    BeginCommandBuffer(cmdBuffer);

    VkEventCreateInfo event_info;
    VkEvent event;
    VkMemoryRequirements mem_req;
    VkResult err;

    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(device(), &event_info, &event);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectMemoryRequirements(device(), VK_OBJECT_TYPE_EVENT, event, &mem_req);
    ASSERT_VK_SUCCESS(err);

    VkMemoryAllocInfo mem_info;
    VkDeviceMemory event_mem;

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.memoryTypeIndex = 0;

    err = m_device->gpu().set_memory_type(mem_req.memoryTypeBits, &mem_info, 0);
    ASSERT_VK_SUCCESS(err);

    err = vkAllocMemory(device(), &mem_info, &event_mem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindObjectMemory(device(), VK_OBJECT_TYPE_EVENT, event, event_mem, 0);
    ASSERT_VK_SUCCESS(err);

    err = vkResetEvent(device(), event);
    ASSERT_VK_SUCCESS(err);

    struct thread_data_struct data;
    data.cmdBuffer = cmdBuffer.obj();
    data.event = event;
    data.bailout = false;
    m_errorMonitor->SetBailout(&data.bailout);
    // Add many entries to command buffer from another thread.
    pthread_create(&thread, &thread_attr, AddToCommandBuffer, (void *)&data);
    // Add many entries to command buffer from this thread at the same time.
    AddToCommandBuffer(&data);
    pthread_join(thread, NULL);
    EndCommandBuffer(cmdBuffer);

    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT) << "Did not receive an err from using one VkCommandBufferObj in two threads";
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    m_errorMonitor->ClearState();
    pipe.CreateVKPipeline(descriptorSet, renderPass());

    msgFlags = m_errorMonitor->GetState(&msgString);

    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_ERROR_BIT);
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

    VkCommandBufferObj dummyCmd(m_device);
    VkDescriptorSetObj descriptorSet(m_device);
    descriptorSet.AppendDummy();
    descriptorSet.CreateVKDescriptorSet(&dummyCmd);

    VkResult res = pipe.CreateVKPipeline(descriptorSet, renderPass());
    /* pipeline creation should have succeeded */
    ASSERT_EQ(VK_SUCCESS, res);

    /* should have emitted a warning: the shader is not SPIRV, so we're
     * not going to be able to analyze it */
    msgFlags = m_errorMonitor->GetState(&msgString);
    ASSERT_TRUE(msgFlags & VK_DBG_REPORT_WARN_BIT);
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

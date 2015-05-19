#include <vulkan.h>
#include <vkDbg.h>
#include "gtest-1.7.0/include/gtest/gtest.h"
#include "vkrenderframework.h"
#include "layers_config.h"

#define GLM_FORCE_RADIANS
#include "glm/glm.hpp"
#include <glm/gtc/matrix_transform.hpp>

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

static const char *bindStateVertShaderText =
        "#version 130\n"
        "vec2 vertices[3];\n"
        "void main() {\n"
        "      vertices[0] = vec2(-1.0, -1.0);\n"
        "      vertices[1] = vec2( 1.0, -1.0);\n"
        "      vertices[2] = vec2( 0.0,  1.0);\n"
        "   gl_Position = vec4(vertices[gl_VertexID % 3], 0.0, 1.0);\n"
        "}\n";

static const char *bindStateFragShaderText =
   "#version 130\n"
   "void main() {\n"
   "   gl_FragColor = vec4(0,1,0,1);\n"
   "}\n";

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
        pthread_mutexattr_t attr;
        pthread_mutexattr_init(&attr);
        pthread_mutex_init(&m_mutex, &attr);
        pthread_mutex_lock(&m_mutex);
        m_msgType = VK_DBG_MSG_UNKNOWN;
        m_bailout = NULL;
        pthread_mutex_unlock(&m_mutex);
    }
    void ClearState()
    {
        pthread_mutex_lock(&m_mutex);
        m_msgType = VK_DBG_MSG_UNKNOWN;
        m_msgString.clear();
        pthread_mutex_unlock(&m_mutex);
    }
    VK_DBG_MSG_TYPE GetState(std::string *msgString)
    {
        pthread_mutex_lock(&m_mutex);
        *msgString = m_msgString;
        pthread_mutex_unlock(&m_mutex);
        return m_msgType;
    }
    void SetState(VK_DBG_MSG_TYPE msgType, const char *msgString)
    {
        pthread_mutex_lock(&m_mutex);
        if (m_bailout != NULL) {
            *m_bailout = true;
        }
        m_msgType = msgType;
        m_msgString.reserve(strlen(msgString));
        m_msgString = msgString;
        pthread_mutex_unlock(&m_mutex);
    }
    void SetBailout(bool *bailout)
    {
        m_bailout = bailout;
    }

private:
    VK_DBG_MSG_TYPE        m_msgType;
    std::string            m_msgString;
    pthread_mutex_t        m_mutex;
    bool*                  m_bailout;
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
    void VKTriangleTest(const char *vertShaderText, const char *fragShaderText, BsoFailSelect failMask);
    void GenericDrawPreparation(VkCommandBufferObj *cmdBuffer, VkPipelineObj &pipelineobj, VkDescriptorSetObj &descriptorSet, BsoFailSelect failMask);

protected:
        VkMemoryRefManager         m_memoryRefManager;
        ErrorMonitor               *m_errorMonitor;

    virtual void SetUp() {
        const char *extension_names[] = {"MemTracker", "ObjectTracker", "Threading"};
        const std::vector<const char *> extensions(extension_names,
                                        extension_names + sizeof(extension_names)/sizeof(extension_names[0]));

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

        // Force layer output level to be >= WARNING so that we catch those messages but ignore others
        setLayerOptionEnum("MemTrackerReportLevel",    "VK_DBG_LAYER_LEVEL_WARNING");
        setLayerOptionEnum("ObjectTrackerReportLevel", "VK_DBG_LAYER_LEVEL_WARNING");
        setLayerOptionEnum("ThreadingReportLevel",     "VK_DBG_LAYER_LEVEL_WARNING");

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
    descriptorSet.CreateVKDescriptorSet(cmdBuffer);
    pipelineobj.CreateVKPipeline(descriptorSet);
    cmdBuffer->BindPipeline(pipelineobj);
    cmdBuffer->BindDescriptorSet(descriptorSet);
}

// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************
// ********************************************************************************************************************

TEST_F(VkLayerTest, CallResetCmdBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
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

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an err after calling ResetCommandBuffer on an active Command Buffer";
    if (!strstr(msgString.c_str(),"Resetting CB")) {
        FAIL() << "Error received was not 'Resetting CB (0xaddress) before it has completed. You must check CB flag before'";
    }
}

TEST_F(VkLayerTest, CallBeginCmdBufferBeforeCompletion)
{
    vk_testing::Fence testFence;
    VK_DBG_MSG_TYPE msgType;
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

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an err after calling BeginCommandBuffer on an active Command Buffer";
    if (!strstr(msgString.c_str(),"Calling vkBeginCommandBuffer() on active CB")) {
        FAIL() << "Error received was not 'Calling vkBeginCommandBuffer() on an active CB (0xaddress) before it has completed'";
    }
}

TEST_F(VkLayerTest, MapMemWithoutHostVisibleBit)
{
    VK_DBG_MSG_TYPE msgType;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory       *mem;
    VkMemoryRequirements *mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;
    size_t         mem_reqs_size   = sizeof(VkMemoryRequirements);
    uint32_t       num_allocations = 0;
    size_t         num_alloc_size  = sizeof(num_allocations);

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
        .memProps       = 0,
        .memPriority    = VK_MEMORY_PRIORITY_NORMAL,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                          &num_alloc_size,
                          &num_allocations);
    ASSERT_VK_SUCCESS(err);

    mem_reqs = new VkMemoryRequirements[num_allocations];
    mem      = new VkDeviceMemory[num_allocations];

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                          &mem_reqs_size,
                          mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs[0].size;

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &(mem[0]));
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, 0, mem[0], 0);
    ASSERT_VK_SUCCESS(err);

    // Map memory as if to initialize the image
    void *mappedAddress = NULL;
    err = vkMapMemory(m_device->device(), mem[0], 0, 0, 0, &mappedAddress);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error while tring to map memory not visible to CPU";
    if (!strstr(msgString.c_str(),"Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT")) {
        FAIL() << "Error received did not match expected error message from vkMapMemory in MemTracker";
    }
}

TEST_F(VkLayerTest, BindInvalidMemory)
{
    VK_DBG_MSG_TYPE msgType;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory       *mem;
    VkMemoryRequirements *mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;
    size_t         mem_reqs_size   = sizeof(VkMemoryRequirements);
    uint32_t       num_allocations = 0;
    size_t         num_alloc_size  = sizeof(num_allocations);

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
        .memProps       = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memPriority    = VK_MEMORY_PRIORITY_NORMAL,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                          &num_alloc_size,
                          &num_allocations);
    ASSERT_VK_SUCCESS(err);

    mem_reqs = new VkMemoryRequirements[num_allocations];
    mem      = new VkDeviceMemory[num_allocations];

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                          &mem_reqs_size,
                          mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs[0].size;

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &(mem[0]));
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory before binding
    vkFreeMemory(m_device->device(), mem[0]);
    ASSERT_VK_SUCCESS(err);

    // Try to bind free memory that has been freed
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, 0, mem[0], 0);
    ASSERT_VK_SUCCESS(err);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error while tring to bind a freed memory object";
    if (!strstr(msgString.c_str(),"Unable to set object")) {
        FAIL() << "Error received did not match expected error message from BindObjectMemory in MemTracker";
    }
}

TEST_F(VkLayerTest, FreeBoundMemory)
{
    VK_DBG_MSG_TYPE msgType;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image, allocate memory, free it, and then try to bind it
    VkImage               image;
    VkDeviceMemory       *mem;
    VkMemoryRequirements *mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;
    size_t         mem_reqs_size   = sizeof(VkMemoryRequirements);
    uint32_t       num_allocations = 0;
    size_t         num_alloc_size  = sizeof(num_allocations);

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
        .memProps       = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memPriority    = VK_MEMORY_PRIORITY_NORMAL,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                          &num_alloc_size,
                          &num_allocations);
    ASSERT_VK_SUCCESS(err);

    mem_reqs = new VkMemoryRequirements[num_allocations];
    mem      = new VkDeviceMemory[num_allocations];

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                          &mem_reqs_size,
                          mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs[0].size;

    // allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &(mem[0]));
    ASSERT_VK_SUCCESS(err);

    // Bind memory to Image object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, 0, mem[0], 0);
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, free memory while still bound to object
    vkFreeMemory(m_device->device(), mem[0]);
    ASSERT_VK_SUCCESS(err);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_WARNING) << "Did not receive an warning while tring to free bound memory";
    if (!strstr(msgString.c_str(),"Freeing memory object while it still has references")) {
        FAIL() << "Warning received did not match expected message from freeMemObjInfo  in MemTracker";
    }
}


TEST_F(VkLayerTest, BindMemoryToDestroyedObject)
{
    VK_DBG_MSG_TYPE msgType;
    std::string     msgString;
    VkResult        err;

    ASSERT_NO_FATAL_FAILURE(InitState());
    m_errorMonitor->ClearState();

    // Create an image object, allocate memory, destroy the object and then try to bind it
    VkImage               image;
    VkDeviceMemory       *mem;
    VkMemoryRequirements *mem_reqs;

    const VkFormat tex_format      = VK_FORMAT_B8G8R8A8_UNORM;
    const int32_t  tex_width       = 32;
    const int32_t  tex_height      = 32;
    size_t         mem_reqs_size   = sizeof(VkMemoryRequirements);
    uint32_t       num_allocations = 0;
    size_t         num_alloc_size  = sizeof(num_allocations);

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
        .memProps       = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
        .memPriority    = VK_MEMORY_PRIORITY_NORMAL,
    };

    err = vkCreateImage(m_device->device(), &image_create_info, &image);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_ALLOCATION_COUNT,
                          &num_alloc_size,
                          &num_allocations);
    ASSERT_VK_SUCCESS(err);

    mem_reqs = new VkMemoryRequirements[num_allocations];
    mem      = new VkDeviceMemory[num_allocations];

    err = vkGetObjectInfo(m_device->device(),
                          VK_OBJECT_TYPE_IMAGE,
                          image,
                          VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                          &mem_reqs_size,
                          mem_reqs);
    ASSERT_VK_SUCCESS(err);

    mem_alloc.allocationSize = mem_reqs[0].size;

    // Allocate memory
    err = vkAllocMemory(m_device->device(), &mem_alloc, &(mem[0]));
    ASSERT_VK_SUCCESS(err);

    // Introduce validation failure, destroy Image object before binding
    vkDestroyObject(m_device->device(), VK_OBJECT_TYPE_IMAGE, image);
    ASSERT_VK_SUCCESS(err);

    // Now Try to bind memory to this destroyted object
    err = vkBindObjectMemory(m_device->device(), VK_OBJECT_TYPE_IMAGE, image, 0, mem[0], 0);
    ASSERT_VK_SUCCESS(err);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error while binding memory to a destroyed object";
    if (!strstr(msgString.c_str(),"Unable to set object")) {
        FAIL() << "Error received did not match expected error message from updateObjectBinding in MemTracker";
    }
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
        FAIL() << "Error received was not 'VkQueueSubmit with fence in SIGNALED_STATE'";
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
        FAIL() << "Error received was not 'VkResetFences with fence in UNSIGNALED_STATE'";
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
        FAIL() << "Error received was not 'Waiting for Unsubmitted Fence'";
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
        FAIL() << "Error received was not 'does not match designated type'";
    }
}

TEST_F(VkLayerTest, RasterStateNotBound)
{
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;

    TEST_DESCRIPTION("Simple Draw Call that validates failure when a raster state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailRaster);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from Not Binding a Raster State Object";
    if (!strstr(msgString.c_str(),"Raster object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Raster object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, ViewportStateNotBound)
{
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;
    TEST_DESCRIPTION("Simple Draw Call that validates failure when a viewport state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailViewport);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from Not Binding a Viewport State Object";
    if (!strstr(msgString.c_str(),"Viewport object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Viewport object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, ColorBlendStateNotBound)
{
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;

    TEST_DESCRIPTION("Simple Draw Call that validates failure when a color-blend state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailColorBlend);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from Not Binding a ColorBlend State Object";
    if (!strstr(msgString.c_str(),"Color-blend object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Color-blend object not bound to this command buffer'";
    }
}

TEST_F(VkLayerTest, DepthStencilStateNotBound)
{
    VK_DBG_MSG_TYPE msgType;
    std::string msgString;

    TEST_DESCRIPTION("Simple Draw Call that validates failure when a depth-stencil state object is not bound beforehand");

    VKTriangleTest(bindStateVertShaderText, bindStateFragShaderText, BsoFailDepthStencil);

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an error from Not Binding a DepthStencil State Object";
    if (!strstr(msgString.c_str(),"Depth-stencil object not bound to this command buffer")) {
        FAIL() << "Error received was not 'Depth-stencil object not bound to this command buffer'";
    }
}

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
        vkCmdSetEvent(data->cmdBuffer, data->event, VK_PIPE_EVENT_COMMANDS_COMPLETE);
        if (data->bailout) {
            break;
        }
    }
    return NULL;
}

TEST_F(VkLayerTest, ThreadCmdBufferCollision)
{
    VK_DBG_MSG_TYPE msgType;
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
    size_t data_size = sizeof(mem_req);
    VkResult err;

    memset(&event_info, 0, sizeof(event_info));
    event_info.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;

    err = vkCreateEvent(device(), &event_info, &event);
    ASSERT_VK_SUCCESS(err);

    err = vkGetObjectInfo(device(), VK_OBJECT_TYPE_EVENT, event, VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS,
                           &data_size, &mem_req);
    ASSERT_VK_SUCCESS(err);

    VkMemoryAllocInfo mem_info;
    VkDeviceMemory event_mem;

    ASSERT_NE(0, mem_req.size) << "vkGetObjectInfo (Event): Failed - expect events to require memory";

    memset(&mem_info, 0, sizeof(mem_info));
    mem_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_info.allocationSize = mem_req.size;
    mem_info.memProps = VK_MEMORY_PROPERTY_SHAREABLE_BIT;
    mem_info.memPriority = VK_MEMORY_PRIORITY_NORMAL;
    err = vkAllocMemory(device(), &mem_info, &event_mem);
    ASSERT_VK_SUCCESS(err);

    err = vkBindObjectMemory(device(), VK_OBJECT_TYPE_EVENT, event, 0, event_mem, 0);
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

    msgType = m_errorMonitor->GetState(&msgString);
    ASSERT_EQ(msgType, VK_DBG_MSG_ERROR) << "Did not receive an err from using one VkCommandBufferObj in two threads";
    if (!strstr(msgString.c_str(),"THREADING ERROR")) {
        FAIL() << "Error received was not 'THREADING ERROR'";
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

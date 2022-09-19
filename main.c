#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "stdio.h"
#include "string.h"
#include "stdlib.h"

    #define DEBUG

const uint32_t windowSize[2] = {800, 600};
const uint32_t queuesNeeded = VK_QUEUE_GRAPHICS_BIT ;
struct QueueFamilyIndices {
    uint32_t graphicsFamily;
    uint32_t presentFamily;
    uint32_t Flags;
    uint32_t presentFlag;
};
struct qHandles {
    VkQueue graphics;
    VkQueue present;
};
struct sChainImgInfo {
    uint32_t swapChainImageCount;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
};
struct fileData {
    unsigned char* code;
    uint32_t codeSize;
};

#ifdef DEBUG
#define VALCNT 1
const char* validationLayers[] = {"VK_LAYER_KHRONOS_validation"};

int setupDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo,VkDebugUtilsMessengerEXT* messenger, VkInstance* instance);
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData);
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
int CheckValidationLayers();
#endif

GLFWwindow* initWindow();
int initVulkan(VkInstance *instance, VkDebugUtilsMessengerEXT* messenger);
int pickPhysicalDevice(VkPhysicalDevice* device, VkInstance instance, VkSurfaceKHR surface);
int isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface);
int findQueueFamilies(VkPhysicalDevice physicalDevice, struct QueueFamilyIndices* indices, VkSurfaceKHR* surface);
int createLogicalDevice(VkPhysicalDevice physicalDevice, VkInstance instance, VkDevice* device, struct qHandles* queue, VkSurfaceKHR* surface);
static inline int createSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, VkImage** image, struct sChainImgInfo* imgInfo, VkSwapchainKHR* swapChain);
static inline int createImageViews(VkDevice device, VkImageView** imageViews, VkImage** images, struct sChainImgInfo* imgInfo);
static inline int createGraphicsPipeline(VkDevice device, struct sChainImgInfo* imgInfo, VkRenderPass* renderPass, VkPipelineLayout* layout, VkPipeline* pipeline );
static inline int createRenderPass(VkDevice device, struct sChainImgInfo* imgInfo, VkRenderPass* renderPass);
static inline int createFrameBuffers(VkDevice device , struct sChainImgInfo* imgInfo, VkImageView** imageViews, VkRenderPass* renderPass, VkFramebuffer* frameBuffers);
static inline int createCommandPool(VkDevice device,VkPhysicalDevice physicalDevice, VkSurfaceKHR* surface , VkCommandPool* commandPool);
static inline int createCommandBuffer( VkDevice device , VkCommandPool pool , VkCommandBuffer* commandBuffer);
static inline int recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkFramebuffer* frameBuffers, VkRenderPass renderPass, struct sChainImgInfo* imgInfo, VkPipeline graphicsPipeline);
static inline int createSyncObects(VkDevice device , VkSemaphore* imgAvailable, VkSemaphore* renderFinished, VkFence* inFlight );

int main(){
    GLFWwindow* window = initWindow();
    if(!window) return -1;

    VkInstance vulkan;
#ifdef DEBUG
    VkDebugUtilsMessengerEXT debugMessenger;
    if(initVulkan(&vulkan, &debugMessenger)) return -1;
#else
    if(initVulkan(&vulkan, NULL)) return -1;
#endif

    VkSurfaceKHR surface;
    if(glfwCreateWindowSurface(vulkan, window, NULL, &surface) != VK_SUCCESS) {
        fprintf(stdout, "ERROR: window surface creation failed");
        return -1;
    }

    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    if(pickPhysicalDevice(&physicalDevice,vulkan,surface)) return -1;

    VkDevice device;
    struct qHandles Queue;
    if(createLogicalDevice(physicalDevice,vulkan,&device, &Queue, &surface)) return -1;

    VkSwapchainKHR swapChain;
    VkImage* swapChainImages;  //Actual ImageLocations (currently in RAM and not VRAM)
    struct sChainImgInfo imgInfo;
    if(createSwapChain( physicalDevice, surface, device, &swapChainImages, &imgInfo, &swapChain)) return -1;

    VkImageView* sChainImageViews;
    if(createImageViews(device,&sChainImageViews, &swapChainImages, &imgInfo)) return -1;

    VkRenderPass renderPass;
    if(createRenderPass(device,&imgInfo, &renderPass)) return -1;

    VkPipelineLayout layout;
    VkPipeline pipeline;
    if(createGraphicsPipeline(device,&imgInfo, &renderPass, &layout, &pipeline)) return -1;

    VkFramebuffer frameBuffers[imgInfo.swapChainImageCount];
    if(createFrameBuffers(device, &imgInfo, &sChainImageViews, &renderPass, frameBuffers )) return -1;

    VkCommandPool commandPool; // contains command buffers
    if(createCommandPool(device, physicalDevice, &surface, &commandPool )) return -1;

    VkCommandBuffer commandBuffer; // contains the actual commands
    if(createCommandBuffer(device, commandPool, &commandBuffer ) ) return -1;

    VkSemaphore imgAvailableSemaphore;
    VkSemaphore renderFinishedSemaphore;
    VkFence inFlightFence;
    if(createSyncObects(device, &imgAvailableSemaphore, &renderFinishedSemaphore, &inFlightFence)) return -1;

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
        vkResetFences(device, 1, &inFlightFence);
        uint32_t imageIndex;
        vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imgAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
        vkResetCommandBuffer(commandBuffer, 0 );
        if(recordCommandBuffer(commandBuffer, imageIndex, frameBuffers, renderPass, &imgInfo, pipeline)) return -1;

        VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        VkSubmitInfo submitInfo = {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &imgAvailableSemaphore,
            .pWaitDstStageMask = &waitStage,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 1,
            .pSignalSemaphores = &renderFinishedSemaphore
        };
        if(vkQueueSubmit(Queue.graphics, 1, &submitInfo, inFlightFence ) != VK_SUCCESS ){
            fprintf(stdout, "ERROR: FAILED TO SUBMIT QUEUE\n");
            return -1;
        }
        VkPresentInfoKHR presentInfo = {
            .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .waitSemaphoreCount = 1,
            .pWaitSemaphores = &renderFinishedSemaphore,
            .swapchainCount = 1,
            .pSwapchains = &swapChain,
            .pImageIndices = &imageIndex,
            .pResults = NULL
        };
        vkQueuePresentKHR(Queue.graphics,&presentInfo);
    }

    vkDestroySemaphore(device, imgAvailableSemaphore, NULL);
    vkDestroySemaphore(device, renderFinishedSemaphore, NULL);
    vkDestroyFence(device, inFlightFence, NULL);
    vkDestroyCommandPool(device, commandPool, NULL);
    for(int i = 0; i < imgInfo.swapChainImageCount; i++) vkDestroyFramebuffer(device,frameBuffers[i], NULL);
    vkDestroyPipeline(device, pipeline, NULL);
    vkDestroyPipelineLayout(device, layout, NULL);
    vkDestroyRenderPass(device, renderPass, NULL);
    for(int i = 0; i < imgInfo.swapChainImageCount; i++) vkDestroyImageView(device,sChainImageViews[i], NULL);

    free(sChainImageViews);
    free(swapChainImages);

    vkDestroySwapchainKHR(device,swapChain,NULL);
    vkDestroySurfaceKHR(vulkan, surface, NULL);
    vkDestroyDevice(device, NULL);

    #ifdef DEBUG
    printf("Debug Messenger Destroying\n");
    DestroyDebugUtilsMessengerEXT(vulkan,debugMessenger,NULL);
    #endif

    vkDestroyInstance(vulkan, NULL);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}




inline GLFWwindow* initWindow(){
    if(!glfwInit())return NULL;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); //disable OpenGL since we're using vulkan
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); //disable resizing as it's too hard to deal with
    return glfwCreateWindow(windowSize[0],windowSize[1],"fucking hell", NULL, NULL);
}

inline int initVulkan(VkInstance *instance, VkDebugUtilsMessengerEXT* messenger){
    VkApplicationInfo appInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pApplicationName = "fucking hell",
    .applicationVersion = VK_MAKE_VERSION(1,0,0),
    .pEngineName = "No Engine",
    .engineVersion = VK_MAKE_VERSION(1,0,0),
    .apiVersion = VK_API_VERSION_1_0,
    };

    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    uint32_t layerCount = 0;
    const char** layerNames = NULL;
    void* next = NULL;

    #ifdef DEBUG
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, NULL);
    VkExtensionProperties extensions[extensionCount];
    vkEnumerateInstanceExtensionProperties(NULL, &extensionCount, extensions);
    
    for(int i = 0; i < extensionCount; i++){
        printf("%s\t",extensions[i].extensionName);
    }
    printf("\n");

    if(!CheckValidationLayers()){
        printf("Validation Layer Unavailable\n");
        return 1;
    }
    layerCount = VALCNT;
    layerNames = validationLayers;
    printf("layer Names are %s\n",layerNames[0]);

    VkDebugUtilsMessengerCreateInfoEXT DebugcreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = debugCallback,
        .pUserData = NULL
    };
    next = &DebugcreateInfo;
    
    uint32_t instanceExtensionCount = glfwExtensionCount +1;
    const char* instanceExtensions[glfwExtensionCount+1];
    memcpy(instanceExtensions,glfwExtensions,sizeof(const char*) *glfwExtensionCount);
    instanceExtensions[glfwExtensionCount] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
    #else
    uint32_t instanceExtensionCount = glfwExtensionCount;
    const char** instanceExtensions = glfwExtensions;
    #endif

    VkInstanceCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions,
        .enabledLayerCount = layerCount,
        .ppEnabledLayerNames = layerNames,
        .pNext = next
    };

    if(vkCreateInstance(&createInfo, NULL, instance) != VK_SUCCESS){
        printf("VkInstance Creation failed\n");
        return 1;
    }
    #ifdef DEBUG
    if(setupDebugMessenger(&DebugcreateInfo, messenger, instance))return 1;
    #endif

    return 0;
}

inline int pickPhysicalDevice(VkPhysicalDevice* device, VkInstance instance, VkSurfaceKHR surface){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance,&deviceCount, NULL);
    if(!deviceCount) {
        fprintf(stdout, "ERROR: No Devices Found\n");
        return 1;
    }
    VkPhysicalDevice devices[deviceCount];
    vkEnumeratePhysicalDevices(instance,&deviceCount, devices);
    for(int i = 0; i < deviceCount; i++){
        if(isDeviceSuitable(devices[i], surface)) {
            *device = devices[i];
            break;
        }
    }
    if(*device == VK_NULL_HANDLE) {
        fprintf(stdout, "ERROR: No Suitable Device Found\n");
        return 1;
    }
    #ifdef DEBUG
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(*device, &props);
    fprintf(stdout, "Selected Device Name: %s\n", props.deviceName);
    fprintf(stdout, "Selected Device Type: %d\n", props.deviceType);
    #endif
    return 0;
}

static inline int checkDeviceExtensionSupport(VkPhysicalDevice device){
    uint32_t extensionCount = 0;
    if(vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, NULL) != VK_SUCCESS) return 1;
    VkExtensionProperties availableExtensions[extensionCount];
    if(vkEnumerateDeviceExtensionProperties(device, NULL, &extensionCount, availableExtensions) != VK_SUCCESS) return 1;
    int requiredExtensionCount = 1;
    const char* requiredExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    for (int i = 0; i < extensionCount; i++)
    {
        for(int j = 0; j < requiredExtensionCount; j++){
            if(strcmp(availableExtensions[i].extensionName,requiredExtensions[j])){
                requiredExtensions[j] = requiredExtensions[--requiredExtensionCount];
                break;
            }
        }
        if(!requiredExtensionCount) break;
    }
    return requiredExtensionCount;
}

inline int isDeviceSuitable(VkPhysicalDevice device, VkSurfaceKHR surface){
    VkPhysicalDeviceProperties props;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &props);
    vkGetPhysicalDeviceFeatures(device, &features);
    uint32_t deviceFlag =(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) & (features.geometryShader);
    VkBool32 presentFlag = 0;
    uint32_t queueFamilyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(device,&queueFamilyCount, NULL);
    for(int i = 0; i < queueFamilyCount; i++){
        vkGetPhysicalDeviceSurfaceSupportKHR(device,i,surface, &presentFlag);
        if(presentFlag) break;
    }
    uint32_t swapChainFlag = (checkDeviceExtensionSupport(device) == 0);
    if(swapChainFlag){
        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device,surface,&formatCount, NULL);
        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device,surface,&presentModeCount, NULL);
        uint32_t surfaceFlag = (formatCount != 0) & (presentModeCount != 0);
        swapChainFlag &= surfaceFlag;
    }
    return presentFlag & deviceFlag & swapChainFlag;
}

inline int findQueueFamilies(VkPhysicalDevice physicalDevice, struct QueueFamilyIndices* indices, VkSurfaceKHR* surface){
    indices->Flags = 0;
    uint32_t exitFlag = 1;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilyProperties[queueFamilyCount];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties);
    for(int i = 0; i < queueFamilyCount; i++){
        VkBool32 presentSupp = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, *surface, &presentSupp);
        if(presentSupp){
            indices->presentFamily = i;
            indices->presentFlag = 1;
        }
        if(queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices->graphicsFamily = i;
            indices->Flags |= VK_QUEUE_GRAPHICS_BIT;
        }
        if((indices->Flags == queuesNeeded) & (indices->presentFlag)){
            exitFlag = 0;
            break;
        }
    }
    return exitFlag;
}

uint32_t filterRepeated(uint32_t* indexes, uint32_t size){
    int offset = 0;
    for(int i = 1; i < size; i++){
        uint32_t flag = 1;
        for(int j = 0; j < (i - offset); j++){
            if(indexes[i] == indexes[j]) {
                flag = 0;
                offset++;
                break;
            }
        }
        if(flag) indexes[i - offset] = indexes[i];
    }
    return size - offset;
}

#define QUEUE_COUNT 2
inline int createLogicalDevice(VkPhysicalDevice physicalDevice, VkInstance instance, VkDevice* device, struct qHandles* queue, VkSurfaceKHR* surface){
    struct QueueFamilyIndices indices;
    if(findQueueFamilies(physicalDevice,&indices,surface)){
        printf("CRITICAL ERROR: QUEUE FAMILIES NOT FOUND\n");
        return 1;
    }
    uint32_t queueIndexes[QUEUE_COUNT] = {indices.graphicsFamily, indices.presentFamily};
    uint32_t queueCount = filterRepeated(queueIndexes, QUEUE_COUNT);
#undef QUEUE_COUNT

    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo* queueCreateInfo = (VkDeviceQueueCreateInfo*)malloc(sizeof(VkDeviceQueueCreateInfo)*queueCount);
    for(int i = 0; i < queueCount; i++){
        queueCreateInfo[i].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo[i].queueCount = 1;
        queueCreateInfo[i].pQueuePriorities = &queuePriority;
        queueCreateInfo[i].queueFamilyIndex = queueIndexes[i];
        queueCreateInfo[i].flags = VK_FALSE;
    }

    VkPhysicalDeviceFeatures deviceFeatures = {VK_FALSE};
    uint32_t deviceExtensionCount = 1;
    const char* deviceExtensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo deviceCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = queueCreateInfo,
        .queueCreateInfoCount = queueCount,
        .pEnabledFeatures = &deviceFeatures,
        .enabledExtensionCount = deviceExtensionCount,
        .ppEnabledExtensionNames = deviceExtensions
    };

    if(vkCreateDevice(physicalDevice, &deviceCreateInfo, NULL, device) != VK_SUCCESS) {
        fprintf(stdout,"ERROR: Logical Device Creation Failed\n");
        free(queueCreateInfo);
        return 1;
    }
#ifdef DEBUG
    fprintf(stdout, "DEBUG: Device Creation Succesful\n");
#endif
    vkGetDeviceQueue(*device,indices.graphicsFamily,0,&(queue->graphics));
    fprintf(stdout, "DEBUG: Got 1st queue\n");
    vkGetDeviceQueue(*device,indices.presentFamily,0,&(queue->graphics));
    fprintf(stdout, "DEBUG: Got 2nd queue\n");

    free(queueCreateInfo);
    return 0;
}

static inline VkSurfaceFormatKHR chooseSwapSurfaceFormat(const VkSurfaceFormatKHR* formats, int size){
    for (int i = 0; i < size; i++) {
        if((formats[i].format == VK_FORMAT_B8G8R8_SRGB) & (formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)){
            return formats[i];
        }
    }
    fprintf(stdout, "WARNING: SRGB NOT SUPPORTED\n");
    return formats[0];
}

static inline int createSwapChain(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, VkDevice device, VkImage** image, struct sChainImgInfo* imgInfo, VkSwapchainKHR* swapChain){
    //get format info
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,surface,&formatCount,NULL);
    VkSurfaceFormatKHR formats[formatCount];
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice,surface,&formatCount,formats);
    VkSurfaceFormatKHR format = chooseSwapSurfaceFormat(formats, formatCount);
    //choose Presentation Mode
    VkPresentModeKHR presMode = VK_PRESENT_MODE_FIFO_KHR;
    //choose ImageSize
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice,surface,&capabilities);
    VkExtent2D extent = capabilities.currentExtent;
    uint32_t imageCount = capabilities.minImageCount +1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
    imageCount = capabilities.maxImageCount;
    }
    //check If present queue is shared
    struct QueueFamilyIndices indices;
    if(findQueueFamilies(physicalDevice, &indices, &surface)) return 1;
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily, indices.presentFamily};
    VkSharingMode sharMode = VK_SHARING_MODE_EXCLUSIVE;
    uint32_t indexCount = 0;
    uint32_t* familyIndices = NULL;
    if(queueFamilyIndices[0] != queueFamilyIndices[1]){
        sharMode = VK_SHARING_MODE_CONCURRENT;
        indexCount = 2;
        familyIndices = queueFamilyIndices;
    }

    VkSwapchainCreateInfoKHR createInfo = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = format.format,
        .imageColorSpace = format.colorSpace,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = sharMode,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = familyIndices,
        .preTransform = capabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = presMode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    if(vkCreateSwapchainKHR(device,&createInfo, NULL, swapChain) != VK_SUCCESS){
        fprintf(stdout, "ERROR: FAILED TO CREATE SWAPCHAIN\n");
        return 1;
    }

    uint32_t swapImgCount;
    vkGetSwapchainImagesKHR(device,*swapChain, &swapImgCount, NULL);
    if((*image = realloc(*image, sizeof(VkImage)*swapImgCount)) == NULL){
        fprintf(stdout,"ERROR: IMAGE REALLOC FAILED\n");
        return 1;
    }
    vkGetSwapchainImagesKHR(device,*swapChain, &swapImgCount, *image);
    imgInfo->swapChainExtent = extent;
    imgInfo->swapChainImageFormat = format.format;
    imgInfo->swapChainImageCount = imageCount;
    return 0;
}

static inline int createImageViews(VkDevice device, VkImageView** imageViews, VkImage** images, struct sChainImgInfo* imgInfo){ 
    uint32_t imageCount = imgInfo->swapChainImageCount;
    if((*imageViews = realloc(*imageViews, sizeof(VkImageView)*imageCount)) == NULL) {
        fprintf(stdout, "ERROR: IMAGE VIEW REALLOC FAILED\n");
        return 1;
    }
    for(int i = 0; i < imageCount; i++){
        VkImageViewCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image = (*images)[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = imgInfo->swapChainImageFormat,
            .components = {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY
            },
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0,
                .levelCount = 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            }
        };
        if(vkCreateImageView(device,&createInfo, NULL, (*imageViews) + i) != VK_SUCCESS){
            fprintf(stdout,"ERROR: FAILED TO CREATE IMAGE VIEW #%d of %d\n", i, imageCount);
            return 1;
        }
    }
    return 0;
}

static inline int readFile(const char* fileName, struct fileData* data){
    FILE* fp = fopen(fileName, "r");
    if(fp == NULL){
        fprintf(stdout, "ERROR: FILE OPEN FAILED FOR %s\n",fileName);
        return 1;
    }
    fseek(fp,0L, SEEK_END);
    size_t res = ftell(fp);
    if((data->code = (unsigned char*)malloc(sizeof(char)*res)) == NULL){
        fprintf(stdout, "ERROR: MALLOC FAILED FOR %s\n",fileName);
        return 1;
    }
    fseek(fp,0L,SEEK_SET);
    if(fread(data->code,sizeof(char),res,fp) != res){
        fprintf(stdout, "ERROR: FILE READ FAILED FOR %s\n",fileName);
        if(data->code) free(data->code);
        return 1;
    }
    if(fclose(fp)){
        fprintf(stdout, "ERROR: FAILURE TO CLOSE FILE %s\n",fileName);
        return 1;
    }
    data->codeSize = res;
    return 0;

}

static inline int createShaderModule(VkDevice device ,struct fileData* shaderCode, VkShaderModule* shader){
    VkShaderModuleCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = shaderCode->codeSize,
        .pCode = (const uint32_t*)shaderCode->code
    };
    if(vkCreateShaderModule(device,&createInfo, NULL, shader)) {
        fprintf(stdout, "ERROR: SHADER MODULE CREATION FAILED\n");
        return 1;
    }
    free(shaderCode->code);
    return 0;
}

static inline int createRenderPass(VkDevice device, struct sChainImgInfo* imgInfo, VkRenderPass* renderPass){
    VkAttachmentDescription colorAttachment = {
        .format = imgInfo->swapChainImageFormat,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef = {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass = {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency = {
            .srcSubpass = VK_SUBPASS_EXTERNAL,
            .dstSubpass = 0,
            .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };

    VkRenderPassCreateInfo createInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    if(vkCreateRenderPass(device, &createInfo, NULL, renderPass) != VK_SUCCESS) {
        fprintf(stdout, "ERROR: RENDER PASS CREATION FAILED\n");
        return 1;
    }

    return 0;
}

static inline int createGraphicsPipeline(VkDevice device, struct sChainImgInfo* imgInfo, VkRenderPass* renderPass, VkPipelineLayout* layout, VkPipeline* pipeline ){
    struct fileData vertShaderCode;
    struct fileData fragShaderCode;
    if(readFile("shaders/vert.spv",&vertShaderCode)) return 1;
    if(readFile("shaders/frag.spv",&fragShaderCode)) return 1;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;
    if(createShaderModule(device,&vertShaderCode,&vertShaderModule)) return 1;
    if(createShaderModule(device,&fragShaderCode,&fragShaderModule)) return 1;

    VkPipelineShaderStageCreateInfo shaderStages[] = {{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_VERTEX_BIT,
        .module = vertShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    },{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
        .module = fragShaderModule,
        .pName = "main",
        .pSpecializationInfo = NULL
    }};

    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    uint32_t dynamicStateCount = 2;
    VkPipelineDynamicStateCreateInfo dynamicStateCreate = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStateCount,
        .pDynamicStates = dynamicStates
    };

    VkPipelineVertexInputStateCreateInfo vertexCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = NULL,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = NULL
    };

    VkPipelineInputAssemblyStateCreateInfo inputCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkViewport viewPort = {
        .x = 0.0f,
        .y = 0.0f,
        .width = imgInfo->swapChainExtent.width,
        .height = imgInfo->swapChainExtent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = imgInfo->swapChainExtent
    };

    VkPipelineViewportStateCreateInfo viewPortCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_CLOCKWISE,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .depthBiasClamp = 0.0f
    };

    VkPipelineMultisampleStateCreateInfo multiCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        .minSampleShading = 1.0f,
        .pSampleMask = NULL,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        .blendEnable = VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp = VK_BLEND_OP_ADD
    };

    VkPipelineColorBlendStateCreateInfo colorBlendCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachment,
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f}
    };

    VkPipelineLayoutCreateInfo layoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 0,
        .pSetLayouts = NULL,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = NULL
    };

    if(vkCreatePipelineLayout(device, &layoutCreateInfo, NULL, layout ) != VK_SUCCESS ) {
        fprintf(stdout, "ERROR: PIPELINE LAYOUT CREATION FAILED\n");
        return 1;
    }

    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = 2,
        .pStages = shaderStages,
        .pVertexInputState = &vertexCreateInfo,
        .pInputAssemblyState = &inputCreateInfo,
        .pViewportState = &viewPortCreateInfo,
        .pRasterizationState = &rasterizerCreateInfo,
        .pMultisampleState = &multiCreateInfo,
        .pDepthStencilState = NULL,
        .pColorBlendState = &colorBlendCreateInfo,
        .pDynamicState = &dynamicStateCreate,
        .layout = *layout,
        .renderPass = *renderPass,
        .subpass = 0,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = -1
    };

    if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, NULL, pipeline ) != VK_SUCCESS ){
        fprintf(stdout, "ERROR: GRAPHICS PIPELINE CREATION FAILED\n");
        return 1;
    }

    vkDestroyShaderModule(device, vertShaderModule, NULL);
    vkDestroyShaderModule(device, fragShaderModule, NULL);
    return 0;
}

static inline int createFrameBuffers(VkDevice device , struct sChainImgInfo* imgInfo, VkImageView** imageViews, VkRenderPass* renderPass, VkFramebuffer* frameBuffers){
    uint32_t imgCount = imgInfo->swapChainImageCount;
    for (uint32_t i = 0; i < imgCount; i++)
    {
        VkFramebufferCreateInfo frameBufferCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = *renderPass,
            .attachmentCount = 1,
            .pAttachments = (*imageViews) +i,
            .width = imgInfo->swapChainExtent.width,
            .height = imgInfo->swapChainExtent.height,
            .layers = 1
        };
        if(vkCreateFramebuffer(device, &frameBufferCreateInfo, NULL, frameBuffers + i ) != VK_SUCCESS ) {
            fprintf(stdout, "ERROR: FAILED TO CREATE FRAME BUFFER NO %d\n", i);
            return 1;
        }
    }
    
    return 0;
}

static inline int createCommandPool(VkDevice device,VkPhysicalDevice physicalDevice, VkSurfaceKHR* surface , VkCommandPool* commandPool){
    struct QueueFamilyIndices indices;
    if(findQueueFamilies(physicalDevice, &indices,  surface) ) {
        fprintf(stdout, "ERROR: FAILED TO FIND QUEUE FAMILIES DURING COMMAND POOL CREATION\n");
        return 1;
    }
    VkCommandPoolCreateInfo poolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = indices.graphicsFamily
    };
    if(vkCreateCommandPool(device, &poolCreateInfo, NULL, commandPool) != VK_SUCCESS ) {
        fprintf(stdout, "ERROR: COMMAND POOL CREATION FAILED\n");
        return 1;
    }
    return 0;
}

static inline int createCommandBuffer( VkDevice device , VkCommandPool pool , VkCommandBuffer* commandBuffer){
    VkCommandBufferAllocateInfo allocInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = pool,
        .commandBufferCount = 1,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY
    };

    if(vkAllocateCommandBuffers(device, &allocInfo, commandBuffer) != VK_SUCCESS ){
        fprintf(stdout, "ERROR: COMMAND BUFFER ALLOCATION FAILED\n");
        return 1;
    }
    return 0;
}

static inline int recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex, VkFramebuffer* frameBuffers, VkRenderPass renderPass, struct sChainImgInfo* imgInfo, VkPipeline graphicsPipeline){
    VkCommandBufferBeginInfo beginInfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = 0,
        .pInheritanceInfo = NULL
    };

    if(vkBeginCommandBuffer(commandBuffer, &beginInfo ) != VK_SUCCESS ){
        fprintf(stdout, "ERROR: FAILED TO BEGIN RECORDING COMMAND BUFFER\n");
        return 1;
    }

    VkClearValue clearVal = {
        .color = {.float32 = {0.0f, 0.0f, 0.0f, 1.0f}}
    };
    VkRenderPassBeginInfo renderPassBeginInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = renderPass,
        .framebuffer = frameBuffers[imageIndex],
        .renderArea = {.offset = {0,0}, .extent = imgInfo->swapChainExtent },
        .clearValueCount = 1,
        .pClearValues = &clearVal
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
    // render Pass body begin

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline );
    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = (float)(imgInfo->swapChainExtent.width),
        .height = (float)(imgInfo->swapChainExtent.height),
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(commandBuffer,0,1,&viewport);

    VkRect2D scissor = {
        .extent = imgInfo->swapChainExtent,
        .offset = {0, 0}
    };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    //render Pass body end
    vkCmdEndRenderPass(commandBuffer);

    if(vkEndCommandBuffer(commandBuffer) != VK_SUCCESS){
        fprintf(stdout, "ERROR: FAILED TO EMD COMMAND BUFFER DURING RECORD\n");
        return 1;
    }
    return 0;
}

static inline int createSyncObects(VkDevice device , VkSemaphore* imgAvailable, VkSemaphore* renderFinished, VkFence* inFlight ){
    VkFenceCreateInfo fenceInfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    VkSemaphoreCreateInfo semaphoreInfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    if(
        (vkCreateSemaphore(device, &semaphoreInfo, NULL, imgAvailable) != VK_SUCCESS) |
        (vkCreateSemaphore(device, &semaphoreInfo, NULL, renderFinished) != VK_SUCCESS) |
        (vkCreateFence(device, &fenceInfo, NULL, inFlight) != VK_SUCCESS)
    ){
        fprintf(stdout, "ERROR: FAILED TO CRETE SYNCRONIZATION OBJECTS\n");
        return 1;
    }

    return 0;
}

////////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------// Debug Stuff
#ifdef DEBUG
inline int CheckValidationLayers(){
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, NULL);
    VkLayerProperties availableLayers[layerCount];
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);
    for(int i = 0; i < VALCNT; i++){
        int layerFound = 0;
        for(int j = 0; j < layerCount; j++){
            if(strcmp(validationLayers[i],availableLayers[j].layerName) == 0){
                layerFound = 1;
                break;
            }
        }
        if(!layerFound) return 0;
    }
    return 1;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {
    
    if(messageSeverity < VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
        fprintf(stderr, "validattion layer: %s\n", pCallbackData->pMessage);
    else
        fprintf(stderr, "WARNING: %s\n",pCallbackData->pMessage);
    return VK_FALSE;
}

inline int setupDebugMessenger(VkDebugUtilsMessengerCreateInfoEXT* createInfo,VkDebugUtilsMessengerEXT* messenger, VkInstance* instance){
    if (CreateDebugUtilsMessengerEXT(*instance, createInfo, NULL, messenger) != VK_SUCCESS){
        return 1;
    }
    return 0;
}

inline VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    PFN_vkVoidFunction func = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    printf("I haven't died yet also the adress is %ld\n",(uint64_t)func);
    if (func != NULL) {
        return ((PFN_vkCreateDebugUtilsMessengerEXT)func)(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
    printf("I haven't died *yet*\n");
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

inline void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
    PFN_vkDestroyDebugUtilsMessengerEXT func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != NULL) {
        printf("Debug Messenger Destroyed\n");
        func(instance, debugMessenger, pAllocator);
    }
}
#endif
//------------------------------------------------------------------------------------------------------------//
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
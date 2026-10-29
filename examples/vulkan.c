// Linux: cc vulkan.c -lvulkan -lX11 -lXi
// Windows: clang vulkan.c -lvulkan-1 -luser32 -lgdi32
// MacOS: TODO

#define SWIN_IMPL
#include "../swin.h"

#include <stdio.h>

#ifdef SWIN_PLATFORM_X11
#define VK_USE_PLATFORM_XLIB_KHR
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_XLIB_SURFACE_EXTENSION_NAME
#elif defined SWIN_PLATFORM_WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#define PLATFORM_SURFACE_EXTENSION_NAME VK_KHR_WIN32_SURFACE_EXTENSION_NAME
#endif
// NOTE: Vulkan headers might not be in include path on Windows and macOS
#include <vulkan/vulkan.h>
#define VK_CHECK(x) do { VkResult err = x; if (err) { printf("Vulkan error: %d\n", err); abort(); } } while(0)

VkInstance instance;
VkPhysicalDevice physicalDevice;
VkDevice device;
VkQueue queue;
VkCommandBuffer commandBuffer;
VkFence fence;
VkSurfaceKHR surface;
VkSwapchainKHR swapchain;
VkImage swapchainImages[16];

void initVulkan();
void initVulkanSurface(Swin swin);
void initVulkanSwapchain(Swin swin);
VkResult acquireVulkanSwapchainImage(u32* outSwapchainImageIndex);

int main() {
    SwinInitInfo init = {};
    init.width = 400;
    init.height = 300;
    Swin swin = swinInit(init);
    swinSetWindowFlags(swin, SwinWindow_Resizable);

    initVulkan();
    initVulkanSurface(swin);
    initVulkanSwapchain(swin);

    while (swinPoll(swin)) {
        // Aquire swapchain image or error
        u32 swapchainImageIndex;
        VkResult swapResult = acquireVulkanSwapchainImage(&swapchainImageIndex);

        // Recreate swapchain on window resize
        if (swinIsWindowResized(swin) || swapResult == VK_SUBOPTIMAL_KHR || swapResult == VK_ERROR_OUT_OF_DATE_KHR) {
            initVulkanSwapchain(swin);
            continue;
        }

        VkCommandBufferBeginInfo cmdBeginInfo = {};
        cmdBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        VK_CHECK(vkBeginCommandBuffer(commandBuffer, &cmdBeginInfo));

        // Transition to clearable layout
        VkImage swapchainImage = swapchainImages[swapchainImageIndex];
        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = swapchainImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Clear screen
        f32 t = swinTime(swin);
        f32 r = t - 2.0f * (i32)(t * 0.5f);
        f32 g = t * 0.7f - 2.0f * (i32)(t * 0.35f);
        f32 b = t * 0.3f - 2.0f * (i32)(t * 0.15f);
        r = r < 1.0f ? r : 2.0f - r;
        g = g < 1.0f ? g : 2.0f - g;
        b = b < 1.0f ? b : 2.0f - b;
        VkClearColorValue color = {r, g, b, 1.0f};
        VkImageSubresourceRange clearRange = {};
        clearRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        clearRange.levelCount = 1;
        clearRange.layerCount = 1;
        vkCmdClearColorImage(commandBuffer, swapchainImage, VK_IMAGE_LAYOUT_GENERAL, &color, 1, &clearRange);

        // Transition to present layout
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = 0;
        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            0, 0, NULL, 0, NULL, 1, &barrier);

        VK_CHECK(vkEndCommandBuffer(commandBuffer));

        // Submit
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        VK_CHECK(vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE));
        VK_CHECK(vkQueueWaitIdle(queue));

        // Present
        VkPresentInfoKHR presentInfo = {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &swapchain;
        presentInfo.pImageIndices = &swapchainImageIndex;
        VK_CHECK(vkQueuePresentKHR(queue, &presentInfo));
        VK_CHECK(vkQueueWaitIdle(queue));
    }
}

void initVulkan() {
    // Instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.applicationVersion = 1;
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;
    // Extensions
    const char* instanceExtensions[] = {
        VK_KHR_SURFACE_EXTENSION_NAME,
        PLATFORM_SURFACE_EXTENSION_NAME, // Inferred from SWIN_PLATFORM macro
    };
    instanceInfo.enabledExtensionCount = 2;
    instanceInfo.ppEnabledExtensionNames = instanceExtensions;
    VK_CHECK(vkCreateInstance(&instanceInfo, NULL, &instance));

    // Physical device
    VkPhysicalDevice physicalDevices[16];
    u32 physicalDeviceCount = 0;
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, NULL));
    VK_CHECK(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices));
    physicalDevice = physicalDevices[0];

    // Queue info
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, NULL);
    VkQueueFamilyProperties queueFamilies[16];
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies);
    u32 queueFamilyIndex = 0;
    for (u32 i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            queueFamilyIndex = i;
            break;
        }
    }
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueInfo = {};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.queueFamilyIndex = queueFamilyIndex;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &queuePriority;

    // Device
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = 1;
    deviceInfo.pQueueCreateInfos = &queueInfo;
    const char* deviceExtensions[] = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    };
    deviceInfo.enabledExtensionCount = 1;
    deviceInfo.ppEnabledExtensionNames = deviceExtensions;
    VK_CHECK(vkCreateDevice(physicalDevice, &deviceInfo, NULL, &device));
    // Queue
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);

    // Command pool
    VkCommandPoolCreateInfo commandPoolInfo = {};
    commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
    VkCommandPool commandPool;
    VK_CHECK(vkCreateCommandPool(device, &commandPoolInfo, NULL, &commandPool));
    // Command buffer
    VkCommandBufferAllocateInfo commandBufferInfo = {};
    commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    commandBufferInfo.commandPool = commandPool;
    commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    commandBufferInfo.commandBufferCount = 1;
    VK_CHECK(vkAllocateCommandBuffers(device, &commandBufferInfo, &commandBuffer));

    // Fence
    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VK_CHECK(vkCreateFence(device, &fenceInfo, NULL, &fence));
}

void initVulkanSurface(Swin swin) {
    SwinPlatformState platformState = swinGetPlatformState(swin);

    // Surface
    #ifdef SWIN_PLATFORM_X11
    VkXlibSurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR;
    info.dpy = platformState.x11.display;
    info.window = platformState.x11.window;
    VK_CHECK(vkCreateXlibSurfaceKHR(instance, &info, NULL, &surface));
    #elif defined SWIN_PLATFORM_WIN32
    VkWin32SurfaceCreateInfoKHR info = {};
    info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    info.hinstance = platformState.win32.instance;
    info.hwnd = platformState.win32.wnd;
    VK_CHECK(vkCreateWin32SurfaceKHR(instance, &info, NULL, &surface));
    #endif
}

void initVulkanSwapchain(Swin swin) {
    VK_CHECK(vkDeviceWaitIdle(device));

    // Capabilities
    VkSurfaceCapabilitiesKHR surfaceCaps;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &surfaceCaps));
    VkExtent2D extent = surfaceCaps.currentExtent;
    if (extent.width == UINT32_MAX || extent.height == UINT32_MAX) {
        SwinSize size = swinGetWindowSize(swin);
        extent.width = size.x;
        extent.height = size.y;
    }

    // Swapchain
    VkSwapchainCreateInfoKHR swapInfo = {};
    swapInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapInfo.surface = surface;
    swapInfo.minImageCount = surfaceCaps.minImageCount;
    swapInfo.imageFormat = VK_FORMAT_B8G8R8A8_UNORM;
    swapInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    swapInfo.imageExtent = extent;
    swapInfo.imageArrayLayers = 1;
    swapInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // Allow clearing swapchain image
    swapInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapInfo.preTransform = surfaceCaps.currentTransform;
    swapInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    swapInfo.clipped = true;
    VkSwapchainKHR oldSwapchain = swapchain;
    swapInfo.oldSwapchain = oldSwapchain;
    VK_CHECK(vkCreateSwapchainKHR(device, &swapInfo, NULL, &swapchain));

    if (oldSwapchain != VK_NULL_HANDLE) vkDestroySwapchainKHR(device, oldSwapchain, NULL);

    // Swapchain images
    u32 swapchainImageCount = 0;
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL));
    VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages));
}

VkResult acquireVulkanSwapchainImage(u32* outSwapchainImageIndex) {
    VkResult res = vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, VK_NULL_HANDLE, fence, outSwapchainImageIndex);

    if (res == VK_SUCCESS || res == VK_SUBOPTIMAL_KHR) {
        VK_CHECK(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX));
        VK_CHECK(vkResetFences(device, 1, &fence));
    }

    return res;
}

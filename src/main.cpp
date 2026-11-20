#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <print>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

#define VMA_IMPLEMENTATION
#include <vk_mem_alloc.h>

#include <glm/glm.hpp>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

constexpr int WIDTH = 800;
constexpr int HEIGHT = 800;

static inline void chk(bool result) {
    if (!result) {
        std::println(std::cerr, "call returned an error");
        exit(EXIT_FAILURE);
    }
}

static inline void chk(vk::Result result) {
    if (result != vk::Result::eSuccess) {
        std::println(std::cerr, "call returned a vulkan error: {}", vk::to_string(result));
        exit(EXIT_FAILURE);
    }
}

static inline void chk(VkResult result) { chk(static_cast<vk::Result>(result)); }

template <typename T> static inline auto chk(vk::ResultValue<T> &&resultValue) -> T {
    if (resultValue.result != vk::Result::eSuccess) {
        std::println(std::cerr, "call returned a vulkan error: {}",
                     vk::to_string(resultValue.result));
        exit(EXIT_FAILURE);
    }

    return std::move(resultValue.value);
}

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

auto main(int argc, char *argv[]) -> int {
    chk(SDL_Init(SDL_INIT_VIDEO));
    chk(SDL_Vulkan_LoadLibrary(nullptr));

    vk::raii::Context context;

    uint32_t instanceExtensionCount = 0;

    char const *const *instanceExtensions =
        SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);

    vk::ApplicationInfo appInfo{
        .pApplicationName = "troglodyte",
        .apiVersion = vk::ApiVersion13,
    };

    vk::InstanceCreateInfo instanceCI{
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    vk::raii::Instance instance = chk(context.createInstance(instanceCI));

    auto physicalDevices = chk(instance.enumeratePhysicalDevices());

    uint32_t physDeviceIdx = 0;
    if (argc > 1) {
        physDeviceIdx = std::stoi(argv[1]);
        assert(physDeviceIdx < physicalDevices.size() && "invalid gpu index");
    }

    auto physicalDevice = std::move(physicalDevices[physDeviceIdx]);

    std::println("Selected device: {}", std::string(physicalDevice.getProperties().deviceName));

    auto queueFamilies = physicalDevice.getQueueFamilyProperties();
    uint32_t gfxQueueFamilyIdx = 0;
    for (size_t i = 0; i < queueFamilies.size(); i++) {
        if (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) {
            gfxQueueFamilyIdx = i;
            break;
        }
    }

    chk(SDL_Vulkan_GetPresentationSupport(*instance, *physicalDevice, gfxQueueFamilyIdx));

    float const queueFamilyPriorities = 1.0;
    vk::DeviceQueueCreateInfo queueCI{
        .queueFamilyIndex = gfxQueueFamilyIdx,
        .queueCount = 1,
        .pQueuePriorities = &queueFamilyPriorities,
    };

    std::vector<char const *> const deviceExtensions = {vk::KHRSwapchainExtensionName};

    vk::StructureChain<vk::PhysicalDeviceFeatures2, vk::PhysicalDeviceVulkan12Features,
                       vk::PhysicalDeviceVulkan13Features>
        featureChain = {
            vk::PhysicalDeviceFeatures2{
                .features = {.samplerAnisotropy = vk::True},
            },
            vk::PhysicalDeviceVulkan12Features{
                .descriptorIndexing = vk::True,
                .shaderSampledImageArrayNonUniformIndexing = vk::True,
                .descriptorBindingVariableDescriptorCount = vk::True,
                .runtimeDescriptorArray = vk::True,
                .bufferDeviceAddress = vk::True,

            },
            vk::PhysicalDeviceVulkan13Features{
                .synchronization2 = vk::True,
                .dynamicRendering = vk::True,
            },
        };

    vk::DeviceCreateInfo deviceCI{
        .pNext = &featureChain.get<vk::PhysicalDeviceFeatures2>(),
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCI,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
    };
    auto device = chk(physicalDevice.createDevice(deviceCI));
    auto queue = device.getQueue(gfxQueueFamilyIdx, 0);

    VmaVulkanFunctions vkFunctions{};
    vkFunctions.vkGetInstanceProcAddr = instance.getDispatcher()->vkGetInstanceProcAddr;
    vkFunctions.vkGetDeviceProcAddr = device.getDispatcher()->vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocatorCI{
        .flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT,
        .physicalDevice = *physicalDevice,
        .device = *device,
        .pVulkanFunctions = &vkFunctions,
        .instance = *instance,
    };

    VmaAllocator allocator;
    chk(vmaCreateAllocator(&allocatorCI, &allocator));

    SDL_Window *window =
        SDL_CreateWindow("troglodyte", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);

    VkSurfaceKHR cSurface;
    chk(SDL_Vulkan_CreateSurface(window, *instance, nullptr, &cSurface));
    vk::raii::SurfaceKHR surface(instance, cSurface);

    auto surfaceCaps = chk(physicalDevice.getSurfaceCapabilitiesKHR(surface));

    int windowWidth = 0;
    int windowHeight = 0;
    chk(SDL_GetWindowSize(window, &windowWidth, &windowHeight));

    vk::Extent2D swapchainExtent = surfaceCaps.currentExtent;
    // on wayland an extent widht of 0xFFFFFFFF means that the surface size will be same as window
    if (surfaceCaps.currentExtent.width == 0xFFFFFFFF) {
        swapchainExtent = {
            .width = static_cast<uint32_t>(windowWidth),
            .height = static_cast<uint32_t>(windowHeight),
        };
    }

    vk::Format const imageFormat = vk::Format::eB8G8R8A8Srgb;
    vk::SwapchainCreateInfoKHR swapchainCI{
        .surface = surface,
        .minImageCount = surfaceCaps.minImageCount,
        .imageFormat = imageFormat,
        .imageColorSpace = vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent = swapchainExtent,
        .imageArrayLayers = 1,
        .imageUsage = vk::ImageUsageFlagBits::eColorAttachment,
        .preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity,
        .compositeAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode = vk::PresentModeKHR::eFifo,
    };
    auto swapchain = chk(device.createSwapchainKHR(swapchainCI));

    auto swapchainImages = chk(swapchain.getImages());

    std::vector<vk::raii::ImageView> swapchainImageViews;
    for (auto &image : swapchainImages) {
        vk::ImageViewCreateInfo viewCI{
            .image = image,
            .viewType = vk::ImageViewType::e2D,
            .format = imageFormat,
            .subresourceRange =
                {
                    .aspectMask = vk::ImageAspectFlagBits::eColor,
                    .levelCount = 1,
                    .layerCount = 1,
                },
        };

        swapchainImageViews.push_back(chk(device.createImageView(viewCI)));
    }
    assert(swapchainImageViews.size() == swapchainImages.size());

    std::vector<vk::Format> depthFormats = {vk::Format::eD32SfloatS8Uint,
                                            vk::Format::eD24UnormS8Uint};
    vk::Format depthFormat = vk::Format::eUndefined;
    for (auto &format : depthFormats) {
        auto props = physicalDevice.getFormatProperties(format);

        if (props.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            depthFormat = format;
            break;
        }
    }

    vk::ImageCreateInfo depthImageCI{
        .imageType = vk::ImageType::e2D,
        .format = depthFormat,
        .extent = {.width = swapchainExtent.width, .height = swapchainExtent.height, .depth = 1},
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = vk::SampleCountFlagBits::e1,
        .tiling = vk::ImageTiling::eOptimal,
        .usage = vk::ImageUsageFlagBits::eDepthStencilAttachment,
        .initialLayout = vk::ImageLayout::eUndefined,
    };

    VmaAllocationCreateInfo allocCI{
        .flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkImage cDepthImage;
    VmaAllocation depthImageAllocation;
    chk(vmaCreateImage(allocator, &(*depthImageCI), &allocCI, &cDepthImage, &depthImageAllocation,
                       nullptr));

    vk::Image depthImage(cDepthImage);

    vk::ImageViewCreateInfo depthViewCI{
        .image = depthImage,
        .viewType = vk::ImageViewType::e2D,
        .format = depthFormat,
        .subresourceRange = {.aspectMask = vk::ImageAspectFlagBits::eDepth,
                             .levelCount = 1,
                             .layerCount = 1},
    };
    auto depthImageView = chk(device.createImageView(depthViewCI));

    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    chk(tinyobj::LoadObj(&attrib, &shapes, &materials, nullptr, nullptr, "suzanne.obj"));

    // vk::DeviceSize const indexCount = shapes[0].mesh.indices.size();
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
    for (tinyobj::index_t &index : shapes[0].mesh.indices) {
        Vertex vert{
            .pos = {attrib.vertices[index.vertex_index * 3],
                    -attrib.vertices[index.vertex_index * 3 + 1],
                    attrib.vertices[index.vertex_index * 3 + 2]},
            .normal = {attrib.normals[index.normal_index * 3],
                       -attrib.normals[index.normal_index * 3 + 1],
                       attrib.normals[index.normal_index * 3 + 2]},
            .uv = {attrib.texcoords[index.texcoord_index * 2],
                   1.0 - attrib.texcoords[index.texcoord_index * 2 + 1]},
        };

        vertices.push_back(vert);
        indices.push_back(indices.size());
    }

    vk::DeviceSize vBufSize = sizeof(Vertex) * vertices.size();
    vk::DeviceSize iBufSize = sizeof(uint16_t) * vertices.size();
    vk::BufferCreateInfo bufferCI{
        .size = vBufSize + iBufSize,
        .usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eIndexBuffer,
    };

    VmaAllocationCreateInfo vBufferAllocCI{
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                 VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                 VMA_ALLOCATION_CREATE_MAPPED_BIT,
        .usage = VMA_MEMORY_USAGE_AUTO,
    };

    VkBuffer cVBuffer;
    VmaAllocation vBufferAlloc;
    VmaAllocationInfo vBufferAllocInfo;
    chk(vmaCreateBuffer(allocator, &(*bufferCI), &vBufferAllocCI, &cVBuffer, &vBufferAlloc,
                        &vBufferAllocInfo));

    vk::Buffer vBuffer(cVBuffer);

    vmaDestroyBuffer(allocator, vBuffer, vBufferAlloc);
    vmaDestroyImage(allocator, cDepthImage, depthImageAllocation);

    vmaDestroyAllocator(allocator);

    SDL_DestroyWindow(window);

    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

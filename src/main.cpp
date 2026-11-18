#include <cassert>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <print>
#include <ranges>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define VULKAN_HPP_NO_EXCEPTIONS
#define VULKAN_HPP_NO_CONSTRUCTORS
#include <vulkan/vulkan.hpp>
#include <vulkan/vulkan_raii.hpp>

// constexpr int WIDTH = 800;
// constexpr int HEIGHT = 800;

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

template <typename T> static inline auto chk(vk::ResultValue<T> &&resultValue) -> T {
    if (resultValue.result != vk::Result::eSuccess) {
        std::println(std::cerr, "call returned a vulkan error: {}",
                     vk::to_string(resultValue.result));
        exit(EXIT_FAILURE);
    }

    return std::move(resultValue.value);
}

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
    for (auto [idx, queueFamily] : std::views::enumerate(queueFamilies)) {
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics) {
            gfxQueueFamilyIdx = idx;
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

    SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
}

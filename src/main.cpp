#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <print>

#include <optional>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <volk.h>

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

static inline void chk(VkResult result) { chk(static_cast<vk::Result>(result)); }

auto main() -> int {
    chk(SDL_Init(SDL_INIT_VIDEO));
    chk(SDL_Vulkan_LoadLibrary(nullptr));

    chk(volkInitialize());

    vk::raii::Context context;

    uint32_t instanceExtensionCount = 0;

    char const *const *instanceExtensions =
        SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);

    vk::ApplicationInfo appInfo{
        .pApplicationName = "troglodyte",
        .apiVersion = vk::ApiVersion13,
    };

    [[maybe_unused]]
    vk::InstanceCreateInfo instanceCI{
        .pApplicationInfo = &appInfo,
        .enabledExtensionCount = instanceExtensionCount,
        .ppEnabledExtensionNames = instanceExtensions,
    };

    vk::raii::Instance instance = chk(context.createInstance(instanceCI));
}

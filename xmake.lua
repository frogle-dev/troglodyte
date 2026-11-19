add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")

set_languages("c++23")
set_warnings("all", "extra", "error", "pedantic")
set_defaultmode("debug")

add_cxxflags("-Wno-error=missing-designated-field-initializers")

add_requires(
  "vulkan-headers",
  "vulkan-memory-allocator",
  "volk",
  "ktx",
  "glm",
  "tinyobjloader",
  "libsdl3"
)

target("troglodyte")
set_kind("binary")
add_files("src/*.cpp")
add_packages(
  "vulkan-headers",
  "vulkan-memory-allocator",
  "volk",
  "ktx",
  "glm",
  "tinyobjloader",
  "libsdl3"
)

add_defines(
  "VMA_STATIC_VULKAN_FUNCTIONS=0",
  "VMA_DYNAMIC_VULKAN_FUNCTIONS=1"
)


if is_mode("debug") then
  set_policy("build.sanitizer.address", true)
  set_policy("build.sanitizer.undefined", true)

  set_runenv("LD_PRELOAD", path.join(os.scriptdir(), "libnodlclose.so"))
  add_runenvs("LSAN_OPTIONS",
    "suppressions=" .. path.join(os.scriptdir(), "lsan_suppressions.txt"))
end

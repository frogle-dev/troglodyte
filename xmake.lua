add_rules("mode.debug", "mode.release", "plugin.compile_commands.autoupdate")

set_languages("c++23")
set_warnings("all", "extra", "error", "pedantic")
set_defaultmode("debug")

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

if is_mode("debug") then
  set_policy("build.sanitizer.address", true)
  set_policy("build.sanitizer.undefined", true)

  add_runenvs("LSAN_OPTIONS", "suppressions=" .. path.join(os.scriptdir(), "lsan_suppressions.txt"))
end

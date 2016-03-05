// ml:ldf += -lvulkan
// ml:run = env VK_LAYER_PATH=/usr/share/vulkan/explicit_layer.d $bin
#include <vector>
#include <sstream>
#include <iostream>
#include <string>
#include <vulkan/vulkan.h>
#include <memory>
#include <functional>
#include <type_traits>

namespace vulkan
{
    inline namespace io
    {
        std::string const& result_str(VkResult result)
        {
            #define ENUMERANT(E) \
                case E: { \
                    static std::string value { #E }; \
                    return value; \
                }
            switch (result) {
                ENUMERANT(VK_SUCCESS)
                ENUMERANT(VK_NOT_READY)
                ENUMERANT(VK_TIMEOUT)
                ENUMERANT(VK_EVENT_SET)
                ENUMERANT(VK_EVENT_RESET)
                ENUMERANT(VK_INCOMPLETE)
                ENUMERANT(VK_ERROR_OUT_OF_HOST_MEMORY)
                ENUMERANT(VK_ERROR_OUT_OF_DEVICE_MEMORY)
                ENUMERANT(VK_ERROR_INITIALIZATION_FAILED)
                ENUMERANT(VK_ERROR_DEVICE_LOST)
                ENUMERANT(VK_ERROR_MEMORY_MAP_FAILED)
                ENUMERANT(VK_ERROR_LAYER_NOT_PRESENT)
                ENUMERANT(VK_ERROR_EXTENSION_NOT_PRESENT)
                ENUMERANT(VK_ERROR_FEATURE_NOT_PRESENT)
                ENUMERANT(VK_ERROR_INCOMPATIBLE_DRIVER)
                ENUMERANT(VK_ERROR_TOO_MANY_OBJECTS)
                ENUMERANT(VK_ERROR_FORMAT_NOT_SUPPORTED)
                default: {
                    static std::string value { "UNKNOWN ERROR" };
                    return value;
                }
            }
            #undef ENUMERANT
        }

        std::string const& physical_type_str(VkPhysicalDeviceType type)
        {
            #define ENUMERANT(E) \
                case VK_PHYSICAL_DEVICE_TYPE_ ## E: { \
                    static std::string value { #E }; \
                    return value; \
                }
            switch (type) {
                ENUMERANT(OTHER)
                ENUMERANT(INTEGRATED_GPU)
                ENUMERANT(DISCRETE_GPU)
                ENUMERANT(VIRTUAL_GPU)
                ENUMERANT(CPU)
                default: {
                    static std::string value { "UNKNOWN PHYSICAL DEVICE TYPE" };
                    return value;
                }
            }
            #undef ENUMERANT
        }

        std::ostream & error()
        {
            return (std::cerr << "\e[0;30;41m ERROR \e[0m ");
        }

        std::ostream & debug(std::string const& msg)
        {
            return (std::cerr << "\e[0;30;44m " << msg << " \e[0m ");
        }

        std::ostream & info()
        {
            return (std::cout << "\e[0;30;42m INFO \e[0m ");
        }

        std::ostream & prompt(std::string const& msg)
        {
            return (std::cout << "\e[0;30;43m " << msg << " \e[0m ");
        }

        template <class T>
        bool input(T & x, std::string const& prompt_msg = "INPUT")
        {
            prompt(prompt_msg);
            for (std::string line; std::getline(std::cin, line); ) {
                std::istringstream ss{std::move(line)};
                if (ss >> x) return true;
                prompt(prompt_msg);
            }
            std::cout << "\n";
            return false;
        }

        void die_unless(VkResult result)
        {
            if (result == VK_SUCCESS) return;
            error() << result_str(result) << "\n";
            throw;
        }
    }

    inline namespace handles
    {
        template <class T>
        using dispatchable_handle = std::unique_ptr<
                std::remove_pointer_t<T>,
                std::function<void (T)>>;

        template <class T>
        using handle_destroyer = void (
                T,
                VkAllocationCallbacks const*);

        template <class T>
        using handle_from_instance_destroyer = void (
                VkInstance,
                T,
                VkAllocationCallbacks const*);

        template <class T>
        using handle_from_device_destroyer = void (
                VkDevice,
                T,
                VkAllocationCallbacks const*);

        using instance_handle = dispatchable_handle<VkInstance>;
        using device_handle = dispatchable_handle<VkDevice>;

        using instance_handle_cref = instance_handle const&;
        using device_handle_cref = device_handle const&;

        template <class T>
        auto handle(
                T raw,
                handle_destroyer<T>* destroyer,
                VkAllocationCallbacks const* alloc={})
        {
            return dispatchable_handle<T>{
                raw,
                [destroyer, alloc] (auto x) {
                    destroyer(x, alloc);
                }
            };
        }

        template <class T>
        auto handle(
                T raw,
                instance_handle_cref h,
                handle_from_instance_destroyer<T>* destroyer,
                VkAllocationCallbacks const* alloc={})
        {
            return dispatchable_handle<T>{
                raw,
                [h=&*h, destroyer, alloc] (auto x) {
                    destroyer(h, x, alloc);
                }
            };
        }

        template <class T>
        auto handle(
                T raw,
                device_handle_cref h,
                handle_from_device_destroyer<T>* destroyer,
                VkAllocationCallbacks const* alloc={})
        {
            return dispatchable_handle<T>{
                raw,
                [h=&*h, destroyer, alloc] (auto x) {
                    destroyer(h, x, alloc);
                }
            };
        }
    }

    inline namespace initialization
    {
        auto instance(
                std::vector<char const*> const& exts={},      // extensions
                std::vector<char const*> const& layers={},    // layers
                VkAllocationCallbacks const* alloc={})
        {
            VkInstanceCreateInfo info {
                .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                .enabledExtensionCount = static_cast<uint32_t>(exts.size()),
                .ppEnabledExtensionNames = exts.data(),
                .enabledLayerCount = static_cast<uint32_t>(layers.size()),
                .ppEnabledLayerNames = layers.data(),
            };
            VkInstance raw;

            die_unless(vkCreateInstance(&info, alloc, &raw));
            return handle(raw, vkDestroyInstance, alloc);
        }

        auto instance_simple(bool debug=true)
        {
            if (!debug) return instance();
            return instance(
                    { VK_EXT_DEBUG_REPORT_EXTENSION_NAME },
                    { "VK_LAYER_LUNARG_standard_validation" });
        }

        auto debug_report(
                instance_handle_cref h,
                PFN_vkDebugReportCallbackEXT callback,
                VkAllocationCallbacks const* alloc={})
        {
            #define FETCH(NAME) \
                static auto const NAME = \
                        (PFN_ ## NAME)vkGetInstanceProcAddr(&*h, #NAME)
            FETCH(vkCreateDebugReportCallbackEXT);
            FETCH(vkDestroyDebugReportCallbackEXT);
            #undef FETCH

            constexpr auto flags =
                VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
                VK_DEBUG_REPORT_ERROR_BIT_EXT;

            VkDebugReportCallbackCreateInfoEXT info {
                .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT,
                .pfnCallback = callback,
                .flags = flags,
            };

            VkDebugReportCallbackEXT raw;
            die_unless(vkCreateDebugReportCallbackEXT(&*h, &info, alloc, &raw));
            return handle(raw, h, vkDestroyDebugReportCallbackEXT, alloc);
        }

        auto debug_report_simple(instance_handle_cref h)
        {
            return debug_report(h, [](
                    auto /*flags*/,
                    auto /*ob_type*/,
                    auto /*ob_source*/,
                    auto /*location*/,
                    auto /*code*/,
                    auto layer,
                    auto message,
                    auto /*userdata*/) -> VkBool32 {
                debug(layer) << message << "\n";
                return false;
            });
        }

        auto physical_devices(instance_handle_cref h)
        {
            std::vector<VkPhysicalDevice> devs;
            uint32_t num_devs;
            die_unless(vkEnumeratePhysicalDevices(&*h, &num_devs, nullptr));
            devs.resize(num_devs);
            die_unless(vkEnumeratePhysicalDevices(&*h, &num_devs, devs.data()));
            return devs;
        }

        auto properties(VkPhysicalDevice dev)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(dev, &props);
            return props;
        }

        auto properties(std::vector<VkPhysicalDevice> devs)
        {
            std::vector<VkPhysicalDeviceProperties> props;
            props.reserve(devs.size());
            for (auto dev: devs)
                props.emplace_back(properties(dev));
            return props;
        }
    }

}
namespace vk = vulkan;

int main()
{
    using namespace vk::io;
    auto instance = vk::instance_simple();
    auto debug = vk::debug_report_simple(instance);

    auto physicals = vk::physical_devices(instance);
    auto props_per_phy = vk::properties(physicals);
    (physicals.size() == 0 ? error() : info()) << physicals.size() << " physicals:\n";

    int i = 0;
    for (auto& props: props_per_phy) {
        prompt(std::to_string(i)) << "[" << vk::physical_type_str(props.deviceType) << "] " << props.deviceName << "\n";
        i++;
    }

    switch (physicals.size()) {
        case 0:
            error() << "no compatible devices.\n";
            return 0;
        case 1:
            i = 0;
            prompt("SELECT A DEVICE:");
            prompt("0") << "\n";
            break;
        default:
            if (!input(i, "SELECT A DEVICE:"))
                return 0;
            break;
    }
}


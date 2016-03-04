// ml:ldf += -lvulkan
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
        }
    }

    inline namespace types
    {
        template <class T>
        using dispatchable_handle = std::unique_ptr<std::remove_pointer_t<T>, std::function<void (T)>>;

        template <class T>
        using dispatchable_handle_destroyer = void (T, VkAllocationCallbacks const*);

        using instance_handle = dispatchable_handle<VkInstance>;
    }

    template <class T>
    auto handle(T raw, dispatchable_handle_destroyer<T>* destroyer, VkAllocationCallbacks const* alloc={})
    {
        return instance_handle{raw, [destroyer, alloc] (auto x) { destroyer(x, alloc); }};
    }

    auto instance(VkInstanceCreateInfo info={}, VkAllocationCallbacks const* alloc={})
    {
        info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        VkInstance raw;
        die_unless(vkCreateInstance(&info, alloc, &raw));
        return handle(raw, vkDestroyInstance, alloc);
    }

    auto physical_devices(instance_handle const& h)
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
namespace vk = vulkan;

int main()
{
    using namespace vk::io;
    auto instance = vk::instance();
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


#ifndef OC_MQTT_TOPIC_H
#define OC_MQTT_TOPIC_H

#include <string>
#include <string_view>
#include <vector>

namespace oc::mqtt {

inline std::vector<std::string> SplitTopic(std::string_view topic) {
    std::vector<std::string> levels;
    std::size_t start = 0;
    while (start <= topic.size()) {
        const std::size_t slash = topic.find('/', start);
        if (slash == std::string_view::npos) {
            levels.emplace_back(topic.substr(start));
            break;
        }
        levels.emplace_back(topic.substr(start, slash - start));
        start = slash + 1;
        if (start == topic.size()) {
            levels.emplace_back("");
            break;
        }
    }
    return levels;
}

inline bool TopicMatchesFilter(std::string_view filter, std::string_view topic) {
    const auto filter_levels = SplitTopic(filter);
    const auto topic_levels = SplitTopic(topic);

    std::size_t topic_index = 0;
    for (std::size_t filter_index = 0; filter_index < filter_levels.size(); ++filter_index) {
        const std::string &filter_level = filter_levels[filter_index];
        if (filter_level == "#") {
            return filter_index + 1 == filter_levels.size();
        }
        if (topic_index >= topic_levels.size()) {
            return false;
        }
        if (filter_level != "+" && filter_level != topic_levels[topic_index]) {
            return false;
        }
        ++topic_index;
    }
    return topic_index == topic_levels.size();
}

} // namespace oc::mqtt

#endif

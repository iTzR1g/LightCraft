#pragma once
#include "launcher/instance.h"
#include <vector>

namespace launcher {

class InstanceManager {
public:
    void refresh();
    const std::vector<Instance>& instances() const { return m_instances; }

    bool create_instance(const std::string& name, const std::string& mc_version);
    bool delete_instance(const std::string& name);
    bool save_instance(const Instance& inst);

    int find(const std::string& name) const;

private:
    std::vector<Instance> m_instances;
};

}

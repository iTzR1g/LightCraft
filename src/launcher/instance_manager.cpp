#include "launcher/instance_manager.h"
#include "core/fs.h"
#include <algorithm>

namespace launcher {

void InstanceManager::refresh() {
    m_instances.clear();
    auto dirs = core::list_dirs(core::instances_dir());
    for (auto& d : dirs) {
        std::string path = (core::instances_dir() / d).string();
        if (core::file_exists(fs::path(path) / "instance.json")) {
            m_instances.push_back(Instance::load(path));
        }
    }
}

bool InstanceManager::create_instance(const std::string& name, const std::string& mc_version) {
    if (find(name) >= 0) return false;
    Instance inst = Instance::create(name, mc_version);
    bool ok = inst.save("");
    if (ok) m_instances.push_back(std::move(inst));
    return ok;
}

bool InstanceManager::delete_instance(const std::string& name) {
    int idx = find(name);
    if (idx < 0) return false;
    core::remove_dir_all(core::instances_dir() / name);
    m_instances.erase(m_instances.begin() + idx);
    return true;
}

bool InstanceManager::save_instance(const Instance& inst) {
    int idx = find(inst.name);
    if (idx < 0) return false;
    m_instances[idx] = inst;
    return inst.save("");
}

int InstanceManager::find(const std::string& name) const {
    for (int i = 0; i < (int)m_instances.size(); i++) {
        if (m_instances[i].name == name) return i;
    }
    return -1;
}

}

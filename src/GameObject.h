#ifndef GAME_OBJECT_H
#define GAME_OBJECT_H

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>
#include "Component.h"

// GameObject represents any object in the engine using a collection of components 
// instead of a complex inheritance tree.
class GameObject {
public:
    explicit GameObject(std::string type, int id = -1)
        : type(std::move(type)), id(id) {}

    // Adds or replaces a component on this object.
    template <typename T, typename... Args>
    T& addComponent(Args&&... args) {
        auto component = std::make_unique<T>(std::forward<Args>(args)...);
        T* raw = component.get();
        components[std::type_index(typeid(T))] = std::move(component);
        return *raw;
    }

    // Returns the component of type T, or nullptr if it doesn't have one.
    template <typename T>
    T* getComponent() {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) {
            return nullptr;
        }
        return static_cast<T*>(it->second.get());
    }

    template <typename T>
    const T* getComponent() const {
        auto it = components.find(std::type_index(typeid(T)));
        if (it == components.end()) {
            return nullptr;
        }
        return static_cast<const T*>(it->second.get());
    }

    template <typename T>
    bool hasComponent() const {
        return components.find(std::type_index(typeid(T))) != components.end();
    }

    template <typename T>
    void removeComponent() {
        components.erase(std::type_index(typeid(T)));
    }

    const std::string& getType() const { return type; }
    int getId() const { return id; }

private:
    std::string type; // object type name for debugging
    int id; // network or object ID

    std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
};

#endif
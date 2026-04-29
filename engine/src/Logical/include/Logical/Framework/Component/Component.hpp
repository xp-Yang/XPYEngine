#ifndef Component_hpp
#define Component_hpp

#include "Base/Common.hpp"

#include <memory>
#include <map>
#include <unordered_map>
#include <string>
#include <vector>

class GObject;
struct Component
{
    Component() = delete;
    Component(GObject *parent) : parent_object(parent) {};
    virtual ~Component() {}

    // virtual void tick(float delta_time) = 0;

public:
    GObject *parent_object; // shared_ptr 导致循环引用
};

#endif // !Component_hpp

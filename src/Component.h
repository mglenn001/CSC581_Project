#ifndef COMPONENT_H
#define COMPONENT_H

// Base class for all components. Each component stores data for one 
// specific feature of a game object (like position or movement).
//
// Components do not know about each other or the GameObject. 
// Systems update multiple components together.
struct Component {
    virtual ~Component() = default;
};

#endif
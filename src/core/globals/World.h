#ifndef _WKT_WORLD_H
#define _WKT_WORLD_H

#include "managers/ECSContext.h"

namespace wkt
{

class World final : public wkt::managers::ECSContext
{
public:
    World(const World&) = delete;
    World(World&&) = delete;
    ~World() = default;

    World& operator=(const World&) = delete;
    World& operator=(World&&) = delete;

public:
    static World& getInstance();

private:
    World() = default;
};

}

#endif
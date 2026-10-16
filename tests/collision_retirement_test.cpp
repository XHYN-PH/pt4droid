#include "engine/physics/collision_retirement.h"
#include "engine/physics/collision_world.h"
#include <cassert>
#include <memory>
#include <iostream>

struct Stage{uint32_t id;};
struct Owner{const Stage* stage;uint32_t generation,owner;};
struct Entry{const Stage* stage;};

int main(){
    pt::CollisionWorld world;
    auto removed=std::make_unique<Stage>(Stage{1}),live=std::make_unique<Stage>(Stage{2});
    auto triangle=[](float x){pt::GeomTriangle t{};t.a={x,0,0};t.b={x+1,0,0};t.c={x,0,1};return t;};
    auto a=triangle(0),b=triangle(3);
    uint32_t first=world.AddOwner("removed"),second=world.AddOwner("live");
    world.AddTriangles(std::span(&a,1),glm::mat4(1),-1,first);world.AddTriangles(std::span(&b,1),glm::mat4(1),-1,second);world.Build();
    std::vector<Owner> owners={{removed.get(),1,first},{live.get(),2,second}};removed.reset();
    auto match=[](const Owner& owner,const Entry& entry){return owner.stage==entry.stage&&owner.generation==entry.stage->id;};
    auto retire=[&](const Owner& owner){world.SetOwnerActive(owner.owner,false);};
    assert(pt::RetireCollisionOwners(owners,std::vector<Entry>{{live.get()}},match,retire));
    assert(owners.size()==1&&world.ActiveTriangles()==1);
    pt::RayHit hit;assert(!world.Raycast({.2f,1,.2f},{0,-1,0},2,hit));assert(world.Raycast({3.2f,1,.2f},{0,-1,0},2,hit));
    // An allocator reusing the same address must still force a fresh build.
    live->id=3;assert(!pt::RetireCollisionOwners(owners,std::vector<Entry>{{live.get()}},match,retire));
    assert(world.OwnerActive(second));
    assert(pt::RetireCollisionOwners(owners,std::vector<Entry>{},match,retire));assert(world.ActiveTriangles()==0);
    std::cout<<"Collision retirement passed: freed stage, real queries, reused address generation\n";
}

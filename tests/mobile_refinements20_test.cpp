#include "engine/render/geometry_bounds.h"
#include "engine/render/vfx_order.h"
#include "engine/render/dynamic_resolution.h"
#include "engine/platform/turnip_profile.h"
#include "engine/physics/collision_retirement.h"
#include "engine/physics/collision_world.h"
#include <random>
#include <numeric>
#include <cassert>
#include <cstdio>
#include <limits>
int main(){
 std::mt19937 rng(20571);std::vector<uint32_t> order;
 struct Draw{int layer,priority;float depth;};std::vector<Draw> draws;
 for(unsigned frame=0;frame<3000;++frame){draws.resize(frame%71+1);for(auto& d:draws)d={int(rng()%3),int(rng()%5),float(int(rng()%30)-15)};
  std::vector<uint32_t> reference(draws.size());std::iota(reference.begin(),reference.end(),0);
  std::stable_sort(reference.begin(),reference.end(),[&](auto a,auto b){auto& x=draws[a];auto& y=draws[b];if(x.layer!=y.layer)return x.layer<y.layer;if(x.layer==2)return x.priority<y.priority;return x.depth>y.depth;});
  pt::UpdateVfxOrder(order,draws);assert(order==reference);pt::UpdateVfxOrder(order,draws);assert(order==reference);
 }
 for(unsigned trial=0;trial<20000;++trial){glm::vec3 lo(float(int(rng()%100)-50),float(int(rng()%100)-50),float(int(rng()%100)-50));glm::vec3 hi=lo+glm::vec3(float(rng()%30+1),float(rng()%30+1),float(rng()%30+1));pt::GeometryBounds b{lo,hi,true};
  glm::vec3 p=glm::mix(lo,hi,glm::vec3(float(rng()%1000)/1000,float(rng()%1000)/1000,float(rng()%1000)/1000));glm::vec3 center(float(int(rng()%100)-50),float(int(rng()%100)-50),float(int(rng()%100)-50));float radius=glm::length(p-center);assert(pt::BoundsTouchSphere(b,center,radius));
 }
 assert(!pt::BoundsTouchSphere({{-5,2,2},{5,3,3},true},{0,0,0},1));assert(pt::BoundsTouchSphere({},glm::vec3(0),0));
 assert(pt::BoundsTouchSphere({{1,0,0},{2,1,1},true},glm::vec3(0),1));
 assert(pt::DynamicFrameBudget(120,60)<pt::DynamicFrameBudget(60,120));assert(pt::DynamicFrameBudget(0,120)==pt::DynamicFrameBudget(120,0));
 assert(pt::DynamicFloorScale(.5,.6)==.5f&&pt::DynamicFloorScale(.5,.4)==.4f&&pt::DynamicFloorScale(.6,.667)==.6f);
 assert(pt::android::SelectTurnipProfile(-1).algorithm==nullptr);assert(std::string_view(pt::android::SelectTurnipProfile(1).algorithm)=="prefer_sysmem");assert(std::string_view(pt::android::SelectTurnipProfile(4).flags)=="tune_small");assert(pt::android::SelectTurnipProfile(999).algorithm==nullptr);
 std::vector<int> owners{1,2,3},entries{1,3};unsigned retired=0;
 assert(pt::RetireCollisionOwners(owners,entries,[](int a,int b){return a==b;},[&](int a){assert(a==2);++retired;}));assert(retired==1&&owners==entries);
 entries={3,1};assert(!pt::RetireCollisionOwners(owners,entries,[](int a,int b){return a==b;},[](int){assert(false);}));assert((owners==std::vector<int>{1,3}));
 // Compare a retained grid after removal/rename with a fresh world. Duplicate
 // triangles exercise equal-distance ray and sphere-contact ordering.
 pt::CollisionWorld retained,fresh;auto dead=retained.AddOwner("old:start"),live=retained.AddOwner("next:hallway");auto current=fresh.AddOwner("current:hallway");
 std::vector<pt::GeomTriangle> triangles;
 for(int x=-4;x<5;++x)for(int z=-4;z<5;++z){pt::GeomTriangle t{};t.a={float(x),0,float(z)};t.b={float(x),0,float(z+1)};t.c={float(x+1),0,float(z)};triangles.push_back(t);}
 retained.AddTriangles(triangles,glm::mat4(1),-1,dead);retained.AddTriangles(triangles,glm::mat4(1),-1,live);fresh.AddTriangles(triangles,glm::mat4(1),-1,current);
 retained.Build();fresh.Build();retained.SetOwnerActive(dead,false);retained.RelabelOwner(live,"next","current");assert(retained.OwnerName(live)==fresh.OwnerName(current));
 for(int trial=0;trial<3000;++trial){glm::vec3 p(float(int(rng()%1000)-500)/100,1.f,float(int(rng()%1000)-500)/100);pt::RayHit a{},b{};bool ha=retained.Raycast(p,{0,-1,0},5,a),hb=fresh.Raycast(p,{0,-1,0},5,b);assert(ha==hb);if(ha){assert(a.distance==b.distance&&a.point==b.point&&a.normal==b.normal);}
  std::vector<pt::SphereHit> aa,bb;retained.SphereCast(p,p-glm::vec3(0,2,0),.2f,aa);fresh.SphereCast(p,p-glm::vec3(0,2,0),.2f,bb);assert(aa.size()==bb.size());for(size_t i=0;i<aa.size();++i)assert(aa[i].distance_sq==bb[i].distance_sq&&aa[i].point==bb[i].point&&aa[i].normal==bb[i].normal&&aa[i].edge==bb[i].edge);
 }
 puts("Test20: 3000 exact VFX orders, 20000 conservative shadow samples, ordered collision retirement/rename and 3000 ray/sweep equivalence cases, target budgets and driver profiles passed");
}

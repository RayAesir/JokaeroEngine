// OS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
// deps
#include <fruit/fruit.h>
#include <mimalloc-new-delete.h>
#include <spdlog/spdlog.h>
// local
#include "app/application.h"
#include "app/ini.h"
#include "engine.h"
#include "events.h"
#include "math/random.h"
#include "options.h"

void PlaceScene(Scene& scene) {
  auto& objects = scene.objects_;
  auto& particles = scene.particles_;
  {
    auto obj = objects.CreateObject("ground");
    obj->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
    // area is 200x200 meters
    obj->SetUniScale(20.0f);
  }

  constexpr float area = 100.0f;
  // buildings
  scene.PlaceStraight("ancient_wall", 8, area, 1.0f);
  scene.PlaceStraight("gray_bricks_wall", 8, area, 1.0f);
  scene.PlaceStraight("harsh_bricks_wall", 8, area, 1.0f);
  scene.PlaceStraight("metal_wall", 8, area, 1.0f);
  scene.PlaceStraight("marble_column", 16, area, 1.0f);
  // plants
  scene.PlaceStraight("melaleuca_alternifolia_a", 24, area, 1.0f);
  scene.PlaceStraight("melaleuca_alternifolia_m", 24, area, 1.0f);
  scene.PlaceStraight("cranberry_bush", 24, area, 1.0f);
  scene.PlaceStraight("hoewa_forsteriana", 24, area, 1.0f);
  scene.PlaceStraight("monsterra_seliciosa", 24, area, 1.0f);
  // rocks
  scene.PlaceStraight("desert_rock", 32, area, 0.7f);
  scene.PlaceStraight("mountain_rock", 32, area, 1.0f);
  // items
  scene.PlaceInAir("backpack", 16, area, 1.2f, 6.0f);
  scene.PlaceStraight("brass_cannon", 4, area, 1.0f);
  scene.PlaceInAir("combat_drone", 8, area, 0.8f, 5.0f);
  scene.PlaceInAir("metal_cube", 32, area, 1.0f, 8.0f);
  scene.PlaceStraight("tire", 16, area, 1.0f);
  scene.PlaceStraight("treasure_chest", 8, area, 1.0f);
  scene.PlaceStraight("wood_table", 8, area, 1.0f);

  // positions of the point lights
  for (int i = 0; i < 64; ++i) {
    // mesh with light source
    glm::vec3 pos = rnd::Vec3(-100.0f, 100.0f, 0.5f, 6.0f, -100.0f, 100.0f);
    auto obj = objects.CreateObject("streaky_metal_lamp");
    obj->SetPosition(pos);
    obj->SetUniScale(0.4f);
    auto& light = scene.lights_.CreatePointLight(false);
    light.position_ = pos;
    light.radius_ = rnd::GetFloat(6.0f, 12.0f);
    light.diffuse_ = rnd::Vec3(0.2f, 1.0f, 0.2f, 1.0f, 0.2f, 1.0f);
    light.intensity_ = rnd::GetFloat(2.0f, 8.0f);
  }
  for (int i = 0; i < 8; ++i) {
    // mesh with light source
    glm::vec3 pos = rnd::Vec3(-50.0f, 50.0f, 0.5f, 6.0f, -50.0f, 50.0f);
    auto obj = objects.CreateObject("copper_lamp");
    obj->SetPosition(pos);
    obj->SetUniScale(1.0f);
    auto& light = scene.lights_.CreatePointLight(true);
    light.position_ = pos;
    light.radius_ = rnd::GetFloat(8.0f, 12.0f);
    light.diffuse_ = rnd::Vec3(0.2f, 1.0f, 0.2f, 1.0f, 0.2f, 1.0f);
    light.intensity_ = rnd::GetFloat(2.0f, 8.0f);
  }
  // characters (animated)
  {
    auto obj = objects.CreateObject("nathan");
    obj->SetPosition(glm::vec3(-5.0f, 0.0f, 0.0f));
    obj->SetUniScale(1.0f);
  }
  {
    auto obj = objects.CreateObject("sophia");
    obj->SetPosition(glm::vec3(5.0f, 0.0f, 0.0f));
    obj->SetUniScale(1.0f);
  }
  {
    auto obj = objects.CreateObject("ciri");
    obj->SetPosition(glm::vec3(0.0f, 0.0f, 2.0f));
    obj->SetUniScale(1.0f);
  }
  {
    auto obj = objects.CreateObject("knight_armor");
    obj->SetPosition(glm::vec3(0.0f, 0.0f, -2.0f));
    obj->SetUniScale(1.0f);
  }
  for (int i = 0; i < 4; ++i) {
    auto obj = objects.CreateObject("many_vertex");
    obj->SetPosition(rnd::Vec3(-50.0f, 50.0f, 4.0f, 8.0f, -50.0f, 50.0f));
    obj->SetEuler(glm::vec3(rnd::AngleRad(0.0f, 8.0f),
                            rnd::AngleRad(0.0f, 180.0f),
                            rnd::AngleRad(0.0f, 8.0f)));
    obj->SetUniScale(1.0f);
  }
  // transparent
  scene.PlaceInAir("transparent_crystal", 12, area, 1.6f, 12.0f);

  particles.CreateParticles("Fire", glm::vec3(0.0, 0.0f, -4.0f));
  particles.CreateParticles("Fire", glm::vec3(0.0, 0.0f, -2.0f));
  particles.CreateParticles("Smoke", glm::vec3(0.0, 0.0f, 0.0f));
  particles.CreateParticles("Smoke", glm::vec3(0.0, 0.0f, 2.0f));
  particles.CreateParticles("Magic", glm::vec3(0.0, 0.0f, 4.0f));
  particles.CreateParticles("Magic", glm::vec3(0.0, 0.0f, 6.0f));
}

fruit::Component<Engine> GetEngineComponent() {
  return fruit::createComponent();
}

int main(int argc, char* argv[]) {
  // OS, console codepage UTF-8
  SetConsoleOutputCP(CP_UTF8);
  SetConsoleCP(CP_UTF8);

  // load Application parameters and Engine options
  ini::Load(opt::set::GetIniDescription(), "engine.ini");
  event::SetKeybinds();

  if (app::init::Application() == false) return 1;

  // extended stack allocation 8MB
  fruit::Injector<Engine> injector(GetEngineComponent);
  Engine& engine = injector.get<Engine&>();

  engine.Setup([](Engine& engine) {
    engine.SetState(State::Enum::kLoading);

    auto& scene = engine.scene_;
    engine.LoadEnvMaps();
    engine.LoadAssets();

    scene.SetEnvTexture("Newport_Loft_8k");
    PlaceScene(scene);

    engine.SetState(State::Enum::kScene);
  });

  // pass render function that use OpenGL and ImGui
  app::Run([&engine]() { engine.Render(); });

  return 0;
}

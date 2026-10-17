// Synthetic FOX2 data through the production parser, Lua 5.1 VM and entity API.
// No game scripts or game files are required.
#include "engine/core/log.h"
#include "game/lua_entity.h"
#include "game/stage_manager.h"

#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>

namespace pt {
bool LogEnabled() { return true; }
void LogWrite(LogLevel, std::string_view text) { std::fprintf(stderr, "%.*s\n", int(text.size()), text.data()); }
}

namespace {
using namespace pt;
using namespace pt::game;

struct Host final : EntityHost {
    Stage stage;
    int changes = 0;
    Stage* FindStage(uint32_t id) override { return id == stage.id ? &stage : nullptr; }
    void OnBodyChanged(Stage& owner, const fox2::Entity&, BodyField) override {
        if (&owner != &stage) throw std::runtime_error("wrong stage received a body change");
        ++changes;
    }
    void OnSetupMessageBox(Stage&, const fox2::Entity&) override {}
};

void Run(LuaVm& vm, std::string_view source) {
    if (!vm.RunChunk("light-condition-regression", {reinterpret_cast<const uint8_t*>(source.data()), source.size()})) {
        throw std::runtime_error("Lua light condition regression failed");
    }
    if (lua_gettop(vm.State()) != 0) throw std::runtime_error("Lua stack leaked");
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 2) throw std::runtime_error("expected synthetic FOX2 fixture");
        std::ifstream input(argv[1], std::ios::binary);
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)), {});
        auto file = std::make_shared<fox2::DataSetFile>();
        if (!file->Load("hallway-fixture.fox2", bytes)) throw std::runtime_error("fixture parse failed");
        Host host;
        host.stage.id = 42;
        auto data = std::make_unique<StageData>();
        data->file = file;
        host.stage.files.push_back(std::move(data));
        LuaVm vm;
        RegisterEntityApi(vm.State(), &host);
        for (const auto& entity : file->Entities()) {
            const std::string name = file->EntityName(entity);
            PushEntity(vm.State(), host.stage, &entity, false);
            lua_setglobal(vm.State(), name.c_str());
        }
        Run(vm, R"lua(
            -- Representative use of the game's condition and body APIs. This is
            -- a regression harness, not a replacement trapLightEnable script.
            local function apply(condition, on)
                for _, light in pairs(condition.lightData) do
                    light:GetDataBody().enable = on
                end
                local count = 0
                for _, model in pairs(condition.modelData) do
                    local body = model:GetDataBodyWithReferrer()
                    body.enable = on
                    body.isVisible = on
                    body.isGeomActive = on
                    count = count + 1
                end
                assert(count == 2)
            end
            apply(disable, false)
            assert(not modelA:GetDataBody().enable and not modelB:GetDataBody().isVisible)
            assert(not modelA:GetDataBody().isGeomActive and not sceneLight:GetDataBody().enable)
            apply(enable, true)
            assert(modelA:GetDataBody().enable and modelB:GetDataBody().isVisible)
            assert(modelA:GetDataBody().isGeomActive and sceneLight:GetDataBody().enable)
            assert(enable.modelData[1] == modelA and enable.modelData[2] == modelB)
            assert(enable.modleData[1] == modelA and enable.modleData[2] == modelB)
        )lua");
        if (host.changes != 14) throw std::runtime_error("expected all model/light body changes to reach the host");
        Run(vm, R"lua(
            assert(#canonical.modelData == 1 and canonical.modelData[1] == canonicalModel)
            assert(#canonicalEmpty.modelData == 0)
            assert(type(legacyEmpty.modelData) == 'table' and next(legacyEmpty.modelData) == nil)
            assert(#shortPath.modelData == 2)
            for _, condition in ipairs({otherScript, missingCallback, wrongCallback, wrongClass, staticTypo, missingList}) do
                assert(condition.modelData == nil)
                -- Missing data must remain an error, not silently become {}.
                assert(not pcall(function() for _ in pairs(condition.modelData) do end end))
            end
            assert(enable.unknownProperty == nil)
            assert(not pcall(function() error('unrelated Lua failure') end))
        )lua");
        // A retired stage handle keeps the existing null semantics.
        host.stage.id = 43;
        Run(vm, "assert(enable.modelData == nil and Entity.IsNull(enable))");
        std::puts("Lua light compatibility passed: real FOX2/VM/API, model/light toggles, list order, canonical/empty precedence, script isolation, missing data and retired stage");
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}

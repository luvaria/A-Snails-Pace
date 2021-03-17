#include "controls_overlay.hpp"
#include "render.hpp"
#include "text.hpp"

bool ControlsOverlay::showControlsOverlay = false;

const vec2 BUTTON_SCALE = vec2(50.f, 50.f);

void ControlsOverlay::addControlsPrompt()
{
    // controls
    const vec2 CONTROLS_POS = vec2(50, 50);
    ECS::registry<ControlPrompt>.emplace(addKeyIcon("C", CONTROLS_POS));
    ECS::registry<ControlPrompt>.emplace(addControlDesc("controls", CONTROLS_POS + vec2(BUTTON_SCALE.x, 0)));
}

void ControlsOverlay::removeControlsPrompt()
{
    for (ECS::Entity e : ECS::registry<ControlPrompt>.entities)
        ECS::ContainerInterface::remove_all_components_of(e);
}

void ControlsOverlay::toggleControlsOverlay()
{
    // flip bool
    showControlsOverlay = !showControlsOverlay;

    if (!addControlsOverlayIfOn())
        removeControlsOverlay();
}

bool ControlsOverlay::addControlsOverlayIfOn()
{
    if (!showControlsOverlay)
        return false;

    // best use png or mesh for non-text graphics

    // controls
    const vec2 CONTROLS_POS = vec2(50, 50);
    addKeyIcon("C", CONTROLS_POS);
    addControlDesc("controls", CONTROLS_POS + vec2(BUTTON_SCALE.x, 0));

    // restart
    const vec2 RESTART_POS = CONTROLS_POS + vec2(0, 60);
    addKeyIcon("R", RESTART_POS);
    addControlDesc("restart", RESTART_POS + vec2(BUTTON_SCALE.x, 0));

    // esc
    const vec2 ESC_POS = RESTART_POS + vec2(0, 60);
    addKeyIcon("Esc", ESC_POS);
    addControlDesc("pause", ESC_POS + vec2(BUTTON_SCALE.x, 0));

    // wasd
    const vec2 WASD_POS = CONTROLS_POS + vec2(300, 0);
    const float WASD_SEP = 10.f;
    addKeyIcon("W", WASD_POS);
    addKeyIcon("A", WASD_POS + vec2(-BUTTON_SCALE.x - WASD_SEP, BUTTON_SCALE.y + WASD_SEP));
    addKeyIcon("S", WASD_POS + vec2(0, BUTTON_SCALE.y + WASD_SEP));
    addKeyIcon("D", WASD_POS + vec2(BUTTON_SCALE.x + WASD_SEP, BUTTON_SCALE.y + WASD_SEP));
    addControlDesc("move", WASD_POS + vec2(2 * BUTTON_SCALE.x + WASD_SEP, 0));

    // space
    const vec2 SPACE_POS = WASD_POS + vec2(250, 0);
    addKeyIcon("Space", SPACE_POS);
    addKeyIcon("", SPACE_POS + vec2(BUTTON_SCALE.x, 0));
    addControlDesc("fall", SPACE_POS + vec2(2 * BUTTON_SCALE.x, 0));

    // fire, camera (mouse controls)
    auto leftClick = ECS::Entity();
    auto rightClick = ECS::Entity();

    std::string key = "colored_mesh";
    ShadedMesh& resource = cache_resource(key);
    if (resource.mesh.vertices.size() == 0)
    {
        resource.mesh.loadFromOBJFile(mesh_path("mouse_icon.obj"));
        RenderSystem::createColoredMesh(resource, "colored_mesh");
    }
    ECS::registry<ShadedMeshRef>.emplace(leftClick, resource, RenderBucket::FOREGROUND);
    ECS::registry<ShadedMeshRef>.emplace(rightClick, resource, RenderBucket::FOREGROUND);
    
    Motion& leftClickMotion = ECS::registry<Motion>.emplace(leftClick);
    Motion& rightClickMotion = ECS::registry<Motion>.emplace(rightClick);
    leftClickMotion.scale = BUTTON_SCALE;
    rightClickMotion.scale = BUTTON_SCALE;
    leftClickMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * BUTTON_SCALE.x;
    rightClickMotion.scale = resource.mesh.original_size / resource.mesh.original_size.x * BUTTON_SCALE.x;
    leftClickMotion.scale.y *= -1;
    rightClickMotion.scale *= -1;
    leftClickMotion.position = SPACE_POS + vec2(250, 0);
    rightClickMotion.position = SPACE_POS + vec2(400, 0);

    addControlDesc("fire", leftClickMotion.position + vec2(BUTTON_SCALE.x, 0));
    addControlDesc("camera", rightClickMotion.position + vec2(BUTTON_SCALE.x, 0));

    ECS::registry<Overlay>.emplace(leftClick);
    ECS::registry<Overlay>.emplace(rightClick);

    return true;
}

void ControlsOverlay::removeControlsOverlay()
{
    for (ECS::Entity e : ECS::registry<Overlay>.entities)
        ECS::ContainerInterface::remove_all_components_of(e);
}

void ControlsOverlay::toggleControlsOverlayOff()
{
    showControlsOverlay = false;
    removeControlsOverlay();
}

ECS::Entity ControlsOverlay::addKeyIcon(std::string name, vec2 pos)
{
    // hijacked from DebugSystem createLine()

    auto entity = ECS::Entity();

    std::string key = "key_icon";
    ShadedMesh& resource = cache_resource(key);
    if (resource.effect.program.resource == 0) {
        // create a procedural circle
        constexpr float z = -0.1f;

        // Corner points
        ColoredVertex v;
        v.color = TITLE_COLOUR;
        v.position = { -0.5, -0.5, z };
        resource.mesh.vertices.push_back(v);
        v.position = { -0.5, 0.5, z };
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5, 0.5, z };
        resource.mesh.vertices.push_back(v);
        v.position = { 0.5, -0.5, z };
        resource.mesh.vertices.push_back(v);

        // Two triangles
        resource.mesh.vertex_indices.push_back(0);
        resource.mesh.vertex_indices.push_back(1);
        resource.mesh.vertex_indices.push_back(3);
        resource.mesh.vertex_indices.push_back(1);
        resource.mesh.vertex_indices.push_back(2);
        resource.mesh.vertex_indices.push_back(3);

        RenderSystem::createColoredMesh(resource, "colored_mesh");
    }

    // Store a reference to the potentially re-used mesh object (the value is stored in the resource cache)
    ECS::registry<ShadedMeshRef>.emplace(entity, resource, RenderBucket::FOREGROUND);

    // Create motion
    auto& motion = ECS::registry<Motion>.emplace(entity);
    motion.angle = 0.f;
    motion.velocity = { 0, 0 };
    motion.position = pos;
    motion.scale = BUTTON_SCALE;

    // add text overlay to key
    const auto SF_CARTOONIST_HAND_BOLD = Font::load(SF_CARTOONIST_HAND_BOLD_PATH);

    ECS::registry<Text>.insert(
        entity,
        Text(name, SF_CARTOONIST_HAND_BOLD, pos)
    );
    Text& text = ECS::registry<Text>.get(entity);
    text.scale = OVERLAY_SCALE;
    text.colour = OVERLAY_COLOUR;
    text.position += vec2(-BUTTON_SCALE.x * 2 / 5, BUTTON_SCALE.y * 3 / 16);

    ECS::registry<Overlay>.emplace(entity);

    return entity;
}

ECS::Entity ControlsOverlay::addControlDesc(std::string desc, vec2 pos)
{
    auto entity = ECS::Entity();

    const auto SF_CARTOONIST_HAND = Font::load(SF_CARTOONIST_HAND_PATH);

    ECS::registry<Text>.insert(
        entity,
        Text(desc, SF_CARTOONIST_HAND, pos)
    );
    Text& text = ECS::registry<Text>.get(entity);
    text.scale = OVERLAY_SCALE;
    text.colour = OVERLAY_COLOUR;
    text.position = vec2(pos.x, pos.y + BUTTON_SCALE.y * 3 / 16);

    ECS::registry<Overlay>.emplace(entity);

    return entity;
}

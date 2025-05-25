# BTZC_game_engine
Bozzy-Thea Zelda-like Collectathon Game Engine. Simple to get off the ground.


## Gallery.

![Screenshot](gallery/Screenshot%202025-05-22%20214844.png)
*Simple OBJ loader with hacky lighting and a flying camera. (2025/05/22)*


## Todo List.

1. Get basic renderer assembled.
    - [x] Get everything stubbed out for simple hdr renderer.
    - [x] Create mesh importer (simple obj meshes for now).
        - [x] Use tinyobjloader to load obj file.
        - [x] Import the mesh into the model-mesh data structre.
        - [x] Upload mesh data with OpenGL.
        - [x] Write render functions.
        - [ ] ~~Bonus points for obj writer as well.~~
            - If I'm gonna do a probuilder-like thing it'll be 
    - [x] Write simple material system.
        - [x] Get ifc and connect it with mesh renderer.
    - [x] Create simple probuilder-like mesh
    - [x] Create simple cylinder mesh (representing player character controller).
    - [x] Create render object structure.
    - [x] Write shaders:
        - [x] Basic Geometry color shader.
        - [x] Postprocessing shader w/ ndc quad code.
    - [x] Rework imgui window drawing to make it control the size of the rendering viewport.
    - [x] Create fly cam.
    - [x] FIX BUGS.
        - [x] Can still click buttons during flycam.
            - Disable imgui mouse and keyboard interactivity when entering (then reenable when exit) the flycam.
        - [x] Game view not receiving input when outside main window.
            - Just force it to only stay in main window. (https://github.com/ocornut/imgui/issues/4624)
        - [x] Can enter into fly cam when right clicking on background.
            - Add flag for whether mouse is hovering over the game viewport specifically.

1. Get jolt physics working with renderer.
    - [x] Setup jolt physics world (simple singlethreaded job system for now).
    - [x] Create CharacterVirtual cylinder (exactly like tuned jolt physics example).
    - [ ] MISSING!!!! Add Pre physics update pass (or use as the pre physics scripts) that runs the ExtendedUpdate() that all the CharacterVirtual objects need to run.
        - [ ] Writing scripts is kiiiinda a bit hard, to maybe having a `datas` builder using another set of static functions for each of the scripts could be good?
            - Maybe it can build both the script order and the datas!
            - [ ] I think having a separate physics datas and render datas is needed.
    - [ ] Create Triangle mesh.
        - Will probs have to extend the obj model loader to include vertex and normal information for the physics world.
    - [x] Create bridge between fixed physics timestep and renderer.
        - [x] Have power to slow down renderer if physics sim is too slow.
            - Also will completely freeze the game for the physics to catch up.
        - [x] Get physics interpolation over to render object via physics engine thing.
        - [ ] Get renderobject to pull in physics object.
            - @WAIT!!!! @PROBLEM: In order for the physics object to calculate the interpolated transform, it needs the interpolation value from the engine, which the phys object doesn't have a reference to!
            - Could have it be inserted into the pre-render functions?
            - [x] Get the overall function structure.
            - [x] Write the script that actually does the tethering.
    - [k] Create gameobjects which create both render objects and physics objects.
        - [x] Create script system (can add script entry into the `LIST_OF_SCRIPTS` macro and then define it somewhere else in the application).
        - [ ] Has these properties.
            - [ ] Random GUID.
            - [ ] String name.
            - [ ] (Optional) a render object.
            - [ ] (Optional) a script to execute before every render tick.
            - [ ] (Optional) a physics object. (@NOTE: Compound colliders will be supported)
            - [ ] (Optional) a script to execute before every physics tick.
        - [ ] If both physics and render object are selected, sync the transform of the render object to the physics object.
            - Perhaps have the render object keep a pointer to the physics object so that it's available during the pre-render script execution?
        - [x] Create managing pool.
    - [ ] Add physics component to gameobjects.

1. Orbit camera for player character.
    - [ ] Get the camera to orbit around player character with `update_frontend_follow_orbit()`

1. Code review.
    - [ ] Fix `@COPYPASTA` tags where there are banks.
        - Do we want to do a pool? A different system?
        - There isn't a way to delete render objects. That needs to get written before implementing the level loading/saving system.
    - [ ] Is the gameobject architecture wanted?
    - [ ] Should refactor to put data together in better ways?
        - [ ] Is this important to think about this early and would it create friction at the expense of performance?

1. Level saving/loading.
    - [ ] Game object serialization to json.
    - [ ] Json to Game object generator.
    - [ ] Load level inside a level as a prefab.
    - [ ] Level hierarchy and sorting.

1. Level authoring tools.

1. Add cascaded shadow maps to renderer.
    - @TODO.

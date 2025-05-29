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
    - [x] MISSING!!!! Add Pre physics update pass (or use as the pre physics scripts) that runs the ExtendedUpdate() that all the CharacterVirtual objects need to run.
        - I chose to add an `on_pre_physics_update()` to physics objects and the ExtendedUpdate() 
    - [x] Create Triangle mesh.
        - Will probs have to extend the obj model loader to include vertex and normal information for the physics world.
        - [x] Update impl name to just `tri_mesh`.
    - [ ] @FUTURE idea: ~~Writing scripts is kiiiinda a bit hard, to maybe having a `datas` builder using another set of static functions for each of the scripts could be good?~~
        - ~~@NOTE: I actually like the `datas` system. It's a small nifty serialization system which is nice.~~
        - ~~Maybe it can build both the script order and the datas!~~
        - [ ] ~~I think having a separate physics datas and render datas is needed.~~
    - [x] Create bridge between fixed physics timestep and renderer.
        - [x] Have power to slow down renderer if physics sim is too slow.
            - Also will completely freeze the game for the physics to catch up.
        - [x] Get physics interpolation over to render object via physics engine thing.
        - [x] Get renderobject to pull in physics object.
            - @WAIT!!!! @PROBLEM: In order for the physics object to calculate the interpolated transform, it needs the interpolation value from the engine, which the phys object doesn't have a reference to!
            - Could have it be inserted into the pre-render functions?
            - [x] Get the overall function structure.
            - [x] Write the script that actually does the tethering.
    - [x] Create gameobjects which create both render objects and physics objects.
        - [x] Create script system (can add script entry into the `LIST_OF_SCRIPTS` macro and then define it somewhere else in the application).
        - [ ] May implement these in the @FUTURE, but atm just ideas: ~~Has these properties.~~
            - [ ] ~~Random GUID.~~
            - [ ] ~~String name.~~
            - [ ] ~~(Optional) a render object.~~
            - [ ] ~~(Optional) a script to execute before every render tick.~~
            - [ ] ~~(Optional) a physics object. (@NOTE: Compound colliders will be supported)~~
            - [ ] ~~(Optional) a script to execute before every physics tick.~~
        - [x] If both physics and render object are selected, sync the transform of the render object to the physics object.
            - Perhaps have the render object keep a pointer to the physics object so that it's available during the pre-render script execution?
        - [x] Create managing pool.
    - [ ] ~~Add physics component to gameobjects.~~

1. Orbit camera and movement for player character.
    - [x] Get the camera to orbit around player character with `update_frontend_follow_orbit()`
        - [x] Get camera to orbit around a point.
        - [x] Get player render object reference and track it.
        - [x] Add automatic turning from follow obj velocity.
            - [x] Tries it once, but it's not right. One direction is biased to be the main direction.
            - [x] Do it correctly!
            - [x] Add override timer for the controls (0.5 secs like AHIT).
    - [x] ~~Create script for input powered player character movement.~~ This isn't working right. I think that I need to program a few movement systems built into the character controller and then have the player insert their inputs like `flat_velocity`, `jump`, etc.
        - [x] Inside the player character movement script:
            - [x] Read input and plug in wanted velocity in `move_character()` function.
            - [x] Set up group of variables that control the stats of the player character.
            - [ ] @FUTURE: ~~BONUS: Get group of variables into an imgui window.~~
        - [ ] ~~Change the charcontroller `set_linear_velocity()` to run this character controller monolithic movement thing.~~
            - There ended up being too much that needed to be taken care of with frontend movement, so a tick-get-set method is being used now!
    - [x] Implement F1 releasing the camera lock when doing orbit.
    - [ ] ~~Align orbit camera to actual camera.~~
        - This isn't necessary, since orbit camera is the "game camera"
    - [x] Set imgui "switch to player cam" button in game view to right aligned.
        - Ok ok I knooooow this wasn't the most important thing to do.
    - [x] BUG FIXES
        - [x] Cylinder erratic movement when sliding along walls.
            - Changed to box shape.
        - [x] Rounding 90 degree angles causes stickiness.
            - Disable back faces fixes this.
        - [x] Player getting stuck when on the ground.
            - Forgot to enable `m_allow_sliding`. Enabling fixed it!
        - [x] Player moves during wasd with flycam.
            - Disable if flycam in pre-physics script.
        - [x] Fix player not being the right height for the character controller.
            - Height is understood to include the radii, so when creating the character, I edit the height so that it is the Jolt definition.
            - Also adds a small assert to make sure that the height is enough to encompass both radii.

1. Some misc pleasing things.
    - [ ] Add prototype default texture for probuilder shapes.
        - [x] Get textures working.
        - [ ] Make and add the grid texture.
    - [ ] Camera orbit following is annoying.

1. Code review.
    - [ ] Fix `@COPYPASTA` tags where there are banks.
        - Do we want to do a pool? A different system?
        - There isn't a way to delete render objects. **That needs to get written before implementing the level loading/saving system.**
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

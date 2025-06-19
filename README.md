# BTZC_game_engine
Bozzy-Thea Zelda-like Collectathon Game Engine. Simple to get off the ground.


## Gallery.

![Screenshot](gallery/Screenshot%202025-05-22%20214844.png)
*Simple OBJ loader with hacky lighting and a flying camera. (2025/05/22)*


## 0.1.0-develop.1 (NEXT VERSION, WIP)

### Adds

- Backface culling.
- 10 second async watchdog (for crashing the program if it falls into an infinite loop).
- Game object serialization/deserialization.
- .exe and window icons.
- TODO vv
- Transform hierarchy for both rendering and physics steps.
- Job system (for fast transform hierarchy updating for now).
- Debug view for collision.
- Render object picking.

### Changes

- Grid shader now displays standable and non-standable surfaces.
- Scripts are now unified, instead of having a list of physics and render scripts.
- Script data is now held by an actual class, extending a new script interface.
- `serialization.h` is deprecated (or rather I don't think it's being used, but this may be useful for a binary data saver?)
- Implementation of render obj, physics obj, and game obj keys are via UUID.
- TODO vv
- Physics objs when created are dummy phys objs and using the deserialize or other creation methods do they actually do something in the physics world.
- ImGui is its own section of the application now.
    - Current plan is to ship it with the final application as well.

### Fixes

- Logger failing format string.
- Not being able to delete render objects.


## Todo List.

1. DONE: Get basic renderer assembled.
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

1. DONE: Get jolt physics working with renderer.
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

1. DONE: Orbit camera and movement for player character.
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

1. DONE: Some misc pleasing things.
    - [x] Add prototype default texture for probuilder shapes.
        - [x] Get textures working.
        - [x] Make and add the grid texture.
            - I found a random one off twitter.

1. DONE: Code review.
    - [ ] ~~Fix `@COPYPASTA` tags where there are banks.~~
        - Do we want to do a pool? A different system?
        - There isn't a way to delete render objects. **That needs to get written before implementing the level loading/saving system.**
    - [x] Rewrite render object pool to allow deleting and uses the same atomic access that physics objects and game objects use.
        - Adds some tech debt with 3 copies of this pool system tho hahahahaha!
    - [x] Is the gameobject architecture wanted?
        - Answer: I think that for now the architecture with the scripting system is fairly good. I think that it works for now and being flexible for any new changes in the future should be the goal, so no rewrite atm.
    - [x] Should refactor to put data together in better ways?
        - [x] Is this important to think about this early and would it create friction at the expense of performance?
            - Answer: It's not important at this stage. We need to be flexible for any changes.

1. DONE: Some misc pleasing things (cont.).
    - [x] Add crouching.
        - [x] Ctrl to crouch.
        - [x] Jump or Ctrl to uncrouch (w/o jumping).
        - [x] Undo crouching when fall off (or go midair in some way)
    - [x] Make F1 allow to enter into the orbit cam as well.
    - [x] Create stalling watchdog (set to 10s as default).
    - [ ] ~~Camera orbit following is annoying.~~
        - Skip for now and try again later. If it's annoying then ig I'll have to rethink how it works.

1. DONE: Level saving/loading.
    - [ ] ~~Actually, there is a modern yaml parser/emitter for C++ so let's use that. (https://github.com/biojppm/rapidyaml)~~
        - Tried it and it was hard to use. I think I will just use nlohmann's json lib again, since it's just super duper easy to use.
    - [x] Remove RapidYAML and insert nlohmann json.
        - I think I just didn't have the patience for getting yaml in there. Perhaps the speed and the readability of the files may have been worth it. Idk.
    - [x] Remove `crashoz/uuid_v4` uuid implementation.
        - The SIMD was causing too many issues so I opted for a (probably slower) implementation, `mariusbancila/stduuid`.
    - [x] Game object serialization to JSON.
        - I think there needs to be some kind of gameobject building block or something to get it all together with???
        - Game object could be serialized to:
            - [x] Name
            - [x] Guid
            - [x] List of Guids that are children (Don't create children, since list is actually flat on the backend).
            - [x] List of render scripts w/ accompanying data.
            - [x] List of physics scripts w/ accompanying data.
            - [ ] ~~Render object (and defer here).~~ DO THIS LATER.
            - [ ] ~~Physics object (and defer here).~~ DO THIS LATER.
    - [x] Write JSON to disk.

1. DONE: Refactor scripts.
    - Thoughts:
        - I think that physics objects and render objects should actually own the phys scripts, render scripts, respectively. Or perhaps maybe just having there be a one, single script and it's being owned by the gameobject. And then, it has its "execution time" which can execute on any one of its own triggering functions.
            - Oh wait! It should be a unified type, that extends from a `Script_ifc` which has all of the hooks defined as empty functions, then each gameobject can have a list of `Script_ifc`'s. Then, each script can choose what to derive in its own header file (which gets pulled into the scripts thing).
            - [x] Do it ^^
                - @NOTE: When defining the different script classes that extend `Script_ifc`, it must be a unique name or else (at least clang-cl) doesn't convolute the function names correctly, and they end up being the same class during execution.

1. DONE: Level saving/loading (cont.)
    - [x] Save and load with JSON:
        - [x] Render object.
        - [x] Physics object.
    - [x] JSON to Game object generator.
    - [ ] ~~Load level inside a level as a prefab.~~
    - [x] Level hierarchy and sorting.
        - [x] Stub out the UI window.
        - [x] List all the gameobject names.
        - [x] Deselect game object when click within scene hierarchy window.
        - [x] Show inspector when a game object is selected.
            - Don't show much, just basic stuff like guid.
        - [x] Create new game objects.
        - [x] Click and drag gameobjects around.
            - Be able to add child to game objects.
        - [x] Add root object ordering.
            - Ig this could be serialized into the scene as well?
            - Just a list of the UUIDs of all root game objects honestly should be fine.
            - [x] Adds another list to handle the ordering of the root nodes (`m_root_level_game_obj_ordering`) and shows the sorting correctly.

1. DONE: Window icon.
    - [x] Add a simple icon to exe and window.
        - [x] Leave instructions for how to create the icon and exe.

1. DONE: Refactor?
    - Thoughts:
        - I need somewhere to create a new empty gameobject!!!
        - [x] Create a callback function.
    - Thoughts on transform hierarchy:
        - The gameobject will have a transform so that the transform hierarchy is possible.
        - Soooo, does that mean that there will be redundant transforms for both the render object and the gameobject? It kinda seems that way. Most gameobjects will have associated render objects. They'll have transforms anyway for the transform hierarchy.
        - But physics objects will control the gameobject transform. Going the other way will pop errors ~~unless the physics object is a kinematic tri-mesh~~ (we're gonna not implement that for now. If a gameobject w/ a physics object is told to move, it will print an error (during gameplay)).
            - Maybe in the future the need for physics objects acting on other physics objects will be wanted. Once that comes, this feature will be implemented. It's just kinda unsure how the physics part of it all will work. Perhaps if a gameobject w/ physics object link is found, then the constraints in the physics engine will be the driving factor and the gameobject transform hierarchy is simply ignored? That could maybe work idk.
        - But yeah, it really seems like especially since representing skeleton mesh bones with gameobjects is gonna be the plan, having gameobjects transforms be the "representation", have physics objects drive the gameobject transform, and render objects read the gameobject transform, that's the way that it should be done.
        - [x] Think about these thoughts ^^
            - How will the skeleton mesh transform propagation work?
                - ANSWER: I think that basically there could be a few levels of detail for calculating the skeletal mesh transforms: (@IQHEWRIHDFKNAXI)
                    - A checkbox of whether to recalc the skeletal mesh's gameobjects for physics objects' step as well.
                        - ~~Level-of-detail for physics calculations.~~ Actually, the arm needs to be completely calculated for the sword (if we wanted swords to update every step but the bodies to update at half rate), so whole body should just be done.
                        - So realtime (limited by the constant frame timing of the 50hz physics)
                    - Level-of-detail for rendering vv
                        - Realtime: every frame is calculated.
                        - Physics copy (only available if physics is enabled): never recalculates for rendering but rather copies/relies on the physics recalculation results.
                            - Or rather uploads the results to the gpu.
                                - This should be fast... right? Idk until we try.
                            - If the physics calc checkbox is unchecked, it's changed to the next one vv
                        - Round-robin timeslice: skeleton is put into a round robin and then X number of animations are processed
                            - This helps keep the burden low.
            - Will there be a dirty flag for local transforms that are changed and then after all changes are made the global transforms are recalculated based off the dirty flags???? I think having a two step approach probably is the way to go, that way the batch change can happen in just one fell swoop.
                - Yes.
                - Rather, there should be a dirty flag for the render step and a dirty flag for the physics step.
                    - ^^ Wait wait, is there even a need for this? I don't think this needs to be done imo. I think just one dirty flag is fine.
                        - Bc dirty flags are set when an update is requested. Mmmmm maybe that update doesn't have to happen immediately?
                        - The results of the skeletal mesh stuff will be stored in the game obj transforms.
                    - These should also be atomic. That way the atomic vars' const refs can be passed around and the whole process can take place over worker threads.
                        - I think it could be like a set of jobs sorted by a breadth first approach so that as soon as one matrix is calculated and the is_dirty flag is turned off, 
        - If decide to transition into a transform hierarchy:
            - [ ] ~~Automatically update world transforms of children gameobjects.~~
            - [ ] @FUTURE: ~~Create job system to easily update transform hierarchy.~~
            - [x] Transition into simple transform hierarchy (that supports double positions).
    - Thoughts on clunkiness of ImGui render:
        - I don't think that the renderer should be in charge of ImGui stuff. It kinda doesn't make sense. The renderer should just be in charge of rendering render objects, and ImGui is a system that touches everything, so it shouldn't be part of the renderer at all. Just a `render_imgui()` function callout and that's it.
        - It doesn't need to be a callout but idk, it's weird how this just kinda happens and it shouldn't.
        - Maybe create a class that does touch everything, and the renderer just holds a reference to it and calls `render_imgui()` from there. That way it's like a callout but the renderer doesn't have to have its hands in every single system it wants to touch.
        - [x] Yeah, this change needs to be made. DO IT.

1. Debug views.
    - [ ] Controlling the transform of objects.
        - [x] Add local transform editing in imgui inspector.
            - [x] Initial.
            - [x] Fix rotation issues.
                - Maybe there needs to be a specific euler angles thing that goes on?
                - [x] If the uuid is different from the previous one, then recalculate the euler angles thing.
                - [x] Force `recalc_euler_angles` if transform gizmo is used.
        - [x] Add imguizmo on selected object.
            - [x] Initial.
            - [x] Fix the upside down issue.
            - [x] Fix not-being-able-to-click issue.
                - Perhaps I could just change it so
                - SOLUTION: There's a provided accept-inputs-from-here function `ImGuizmo::SetAlternateWindow()`
            - [x] Add changing the transform space of the gizmo.
                - Keep it universal tho.
        - [x] @BUGFIX: Transform gizmo starts in world space even tho dropdown starts in local space. Decide on one!
        - [ ] @TEST: The local and global object transform hierarchy stuff!
            - Doesn't work atm for some reason.
            - Ok it looks like it's trying to work but it's failing miserably.
                - It doesn't update when
            - [x] Works w position and scale.
            - [x] Works w rotation?
                - Just use the delta matrix for rotations but not for positions (bc that should be delta pos calculated manually).
            - [x] Fix rotation being funky for children.
                - Scale and position work fine, but rotation is odd.
                - Turns out it was the `glm_quat_mul()` order issue w/ p and q.
            - [ ] Fix parenting and local transforms not getting automatically changed correctly.
    - [ ] Collision represented as green wireframe triangles.
        - Depth test enabled.
        - Drawn after postprocessing step that tonemaps hdr->ldr.
        - Sample hdr depth buffer and compare it to all the lines drawn, and use different colors for if in front or behind (i.e. more transparent if behind).
            - [ ] Change hdr depth buffer from renderbuffer to texture (@NOTE: There will be other shader tricks using the depth buffer like water, so this change will not just be for debug effects).
    - [ ] Object picking.
        - Read from picking buffer and use id to select the certain game object.
        - [ ] Only highlight that gameobject as the selected one. That's all this has to do.
    - [ ] Redraw model of selected object as pulsing-color wireframe.
        - Using same system/groundwork of collision, with same depth testing stuff.
    - [ ] @FUTURE: ~~Wireframe view for everything.~~
        - Color by a color generated by their uuids.
    - [ ] @FUTURE: ~~Follow path for player character.~~
        - Select a character and enable follow path debug view.
        - Over the past 500 moved/active physics steps, draw a cube that represents the collision shape and transform of where the character was.
    - [ ] Ch ch ch changes.
        - [ ] Only expand hierarchy tree when double click or use arrow.
        - [ ] Remove the between hidden buttons for rearranging until rearranging mode. That way arrow key navigation isn't weird.

1. Lower priority refactor?
    - Thoughts on phys obj creation system:
        - Forcing that assymetrical phys obj creation method (w/ `create_physics_object_from_serialization()`) really sucks.
        - I think that you should be able to create a physics object that's empty, and then after that do `scene_serialize()` and create a default physics object in the type you're looking for, or something bc this really ain't it.
        - Idk. Pre-packing defaults for the physics objects does seem like a bit of a hassle ngl. (i.e. thinking about writing static json premade thingies for every new type you got). Tho honestly that might just be a fine way of doing things ig. idk!
            - Also, maybe creating the physics objects doesn't need to happen immediately anyway, bc the physics objects will just be created when the game is starting to be played anyway, not during level authoring time.
        - Hmmm, also seems like there needs to be a `scene_serialize()` thing for the impl of the thingies.
        - [ ] Think about these thoughts ^^
            - I think that this kinda change would be good. Like changing the `create_char_controller()` thing to be more of a decorator function and creating an abstract form physics thing that has no impl would be good.

1. Good pause point.
    - [ ] Release v0.1.0-develop.1

1. Unity to this engine migration.
    - @TODO.

1. Level authoring tools.
    - [ ] Create, duplicate, delete game objects.
        - Duplicating copies the serialization of the game object, nullifies the guid, then recreates the object.
    - [ ] In-scene object picker.
        - Render object to the corresponding gameobject.
    - [ ] Prefab creator.
    - [ ] Grid alignment.
    - [ ] Transform gizmos.
        - Use Imguizmo.
        - Also allow for G for translate, R for rotate, and S for scale.
        - Also (while not using the gizmo) allow for W,E,R to change to 
    - [ ] Orthographic view.
        - 4-pane view kinda like what maya has.
            - 1st, main pane is colored, full view
            - 2-4 are wireframe, colored-by-id views that are orthographic.
        - Wherever the moving gizmo is is the focus point.
        - Use scroll wheel to adjust before/after depth for each axis.
        - Display the camera view axes as boxes showing the depth of each capture.

1. Add cascaded shadow maps to renderer.
    - @TODO.
    - [ ] Add player shadow as well, but using a different camera setup.
        - Drop shadow.

1. Skeletal animations using compute shaders.
    - [ ] See `@IQHEWRIHDFKNAXI` for levels of detail and round-robin system.
    - [ ] Create buffers to store the deformed models.
        - 1 per skinned mesh thingy.
    - [ ] Add support for GLTF2 filetype for models.
    - [ ] IK legs.
    - @TODO.

1. Good pause point.
    - [ ] Release v0.1.0-develop.2

1. Work on movement and gameplay stuff (not really engine related for this)!
    - Base movement principles from a hat in time.
    - Use animations to really show the movement stuff.
    - @TODO.

1. Good pause point.
    - [ ] Release v0.1.0

1. PBR Implementation.
    - [ ] Simple direct lighting model.
        - Use Shadowmaps as well.
    - [ ] Cubemap reflection probes.
        - Global IBL prefilter map first.
        - Statically placed reflection probes.
            - Static reflection capture,
            - Or incremental reflection capture (round robin of ones in view, then generate prefilter maps amortized).
            - Have tool representation be a sphere that circumscribes the cube of the near field cutoff of the reflection captures.
            - If moved during play mode, start spitting out errors (probe moved from initial position!!!)
    - [ ] Adaptive SH irradiance probes.
        - [ ] Learn how Spherical Harmonics work.
        - [ ] Research how to reduce light bleeding when sampling irradiance probes.
        - Global IBL irradiance map first (or do this one w SH as well???)
        - Procedurally placed and yeah good luck (Refer to https://unity.com/blog/engine-platform/new-ways-of-applying-global-illumination-in-unity-6)

1. Good pause point.
    - [ ] Release v0.2.0-develop.0

1. Graphics library procedure abstraction.
    - [ ] FIRST: Check inside the `renderer_impl_win64.h/.cpp` to see what different things OpenGL uses and pull those out.
        - GOAL: Have a gfx abstraction layer so that `renderer.h/.cpp` can just define the renderer using this kinda code:
            ```
            unique_ptr<Gfx_abstraction> gfx_implementation{ new gfx_impl_opengl46 };
            Renderer main_renderer{ std::move(gfx_implementation) };
            ```
        - GOAL cont: And then work on the renderer for vulkan!
    - [ ] Create/delete framebuffers.
    - [ ] Create/delete images, views, and samplers.
    - [ ] Run mipmapping generation.

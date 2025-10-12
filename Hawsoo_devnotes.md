# Hawsoo's Dev notes.

## Console window implementation.

- There needs to be like a way to retrieve all the data from the logger. Or maybe there could just be a `get_head_and_tail()` func with then just scanning the applicable rows.
- Oh and then a clear logs func.
- For disabling the logger, a message could just get sent "Switching to in-editor console logger" and then `set_logging_print_mask(NONE)` could get called, and then undone when the window is gone.

- Aaaaggghhh it looks like there's not gonna be anymore ansi coloring in the console bc it's kinda haaaarrd ^_^;

- Ok this seems done for now.

- NEVERMIND!!! The logs printed are off.
    - [x] Fixed.


## Animation authoring tool window.

- Gonna use blender-style context switching like they got on the top bar.
    - Well, except it will be in the main menubar as like a "Context: Level Editor"

> I just learned about `ImGui::PushItemWidth(ImGui::GetFontSize() * -10)`. Put it after beginning a new window and it will align all of the widgets to a constant width instead of making them take up 2/3rds of the window width which was hecka annoying.

- So on a bit of a UI adventure tangent, but I decided there will be the ability to have multiple levels in the hierarchy. Each level will be represented by a root object in the hiearachy with an empty gameobject any kind of transform (ideally just position is changed here).
    - Or maybe it could just be a dropdown like in Unity???
    - Or maybe we could just ignore it all and just change the UI to have one level loaded.
        - Like grayed out or something.

- [x] Get simple context switching.

- Dang, like ImGui code just is monstrous how large it gets. Crazy.

- [x] Make window IDs separate with context switching.

- [x] Add placeholder for:
    - Timeline thingy.
    - Model selector.

- I'm starting to use the living room compy which it would be nice to have an imgui zoom thing.
    - [x] Have a `direct_lookup_table` for the input handler.
    - [x] Implement zoom with ctrl+(-/=)

- These are the things I want the anim authoring tool to be able to handle:
    - Inside the timeline:
        - Temporary state variable overrides.
            - `in_deflect_window : bool`
            - `lerp_to(x) : float` <- @NOTE: Happens over the course of the bar.
        - Events.
            - `play_sfx_fart()` <- @NOTE: Only happens on rising edge of event.
        - Inserting hurtboxes.
            - `use_hurtbox(x)` <- @NOTE: While in this bar, displays previously authored hurtbox.
    - Inside the hit/hurtbox creator:
        - Hitbox/Hurtbox create capsules, add tags, and assign to 1-2 bones.
            - Tags are used for enabling/disabling during timeline.
            - Hitboxes are default on, and hurtboxes are default off.

> Yknow, it really just feels like the best kind of developing is when I play it like Sudoku. Just look at stuff I wanna change and go and do it. Instead of getting overwhelmed, pull back my pace and just take it in a relaxed, slow way.

- I'm just vibe coding rn. Setting in different things and stuff.

- I need to change the region creation to change to non-clicking (so Shift+a like blender it is).

> It really seems to be that game dev and software dev for this matter should be very therapeutic. An experience where one simply sits down and starts vibing w the code. I am just having a blast w the time I spend just sitting and thinking. just sitting and working wo any convos or distractions. I think in the past month I’ve learned so much about how I can be effective thru pacing myself w running and being effective there. its been super fun.

- for this upcoming part it seems like getting the model selector working and connected to some model rendering in the main view is needed. perhaps have a render object that you use? or maybe there needs to be some kind of if statement to switch between level editor/gameplay and the animation editor.

> some thoughts  about the player and animation state machine. it seems like there should be a small set of states (eg: idle, run, jump, knockback, dodge, guard, attack, sheath, unsheath) but these can have many different variations, and before entering the state a variation key is inserted. for example, having "bool weapon_drawn" or "int variation" and this could change the type of attack, breathing style for unsheathing, and guarding (ie midair guarding, sprinting and guarding to a stop) and stuff. then this info could get ingested by the game to generate anim strings. this perhaps may make a lot of work but this should simplify the state machine a lot!!
- ^^^REMEMBER FOR @FUTURE^^^

- Ok so for this next part I need to create a specific script that accesses a global block of data for the animator editor stuff.
    - First thing we'll do is get the animated model to switch depending on the selected model and then select the playing animation and the position in the animation to play depending on the controls.

- Yoooo the tool kinda feels complete as soon as the animations just played with the timeline. Hahahaha feels kinda unreal.

- I think the next thing to work on here is getting the event system all put together and saved into a json file.
    - There also needs to be a main struct to work with that will go well with the animator and the editor.
    - And then a serialization-ifc attached to this struct I think.

- In the `.btafa` file, I ended up putting the key "animated_model_name" which connected to the animated model. I think that this one-to-one connection might not have been the best tho. Idk... I'll have to refactor in the future if needed.

> Mmmmm for some reason I keep on feeling like I'm done with this tool, but I can't really use it for anything lol.

> So it has the ability to create and edit regions and ctrl items, I think that now I just need to match the control items to what they're actually mutating.

- Here's what I want mutated.
    > @NOTE: The below are parts of the runtime data. It should have a copy made and the animator writes to this. Write values if `w` or override values if `o`.
    - Floats
        - Set like this: `float_name:w:3.14444` (On the rising edge of the region, the float `float_name` is set to 3.14444 and writes the value (`w`) persistently.)
        - Interp persistently like this: `float_name:w:0.0->3.5` (For the duration of the region, the float `float_name` lerps from 0.0 to 3.5, and it writes the value (`w`) persistently.)
    - Bools
        - Override like this: `bool_name:o:true` (For the duration of the region, the bool `bool_name` is set to true, but only overwritten, not persistent (`o`).)
    - Rising-edge events
        - Show 

    - Float names
        - model_opacity: range(0-1), default(1.0)
        - turn_speed: range(>=0), default(0.0)
        - move_speed: range(∞), default(0.0)  <<@NOTE: `move_speed` will cause character to start moving along the facing direction. Knockback is a negative number since this is moving back.
        - gravity_magnitude: range(∞), default(1.0)  <<@NOTE: Gravity is (0.0, -98.0, 0.0), but this value gets multiplied by gravity to get actual gravity applied to this character.
        - 

    - Bool names
        - is_parry_active: default(false)
        - can_move_exit: default(true)
        - can_guard_exit: default(true)
        - can_attack_exit: default(true)
        - blade_has_mizunokata: default(false)
        - blade_has_honoonokata: default(false)
        - show_hurtbox_bicep_r: default(false)
        - hide_hitbox_leg_l: default(false)

    - Rising-edge event names
        - play_sfx_footstep
        - play_sfx_ready_guard
        - play_sfx_blade_swing
        <!-- - play_sfx_blade_hit_flesh -->  <<@NOTE: These are going to be called from world interaction, not animation.
        <!-- - play_sfx_blade_hit_stone -->
        - play_sfx_hurt_vocalize_human_male_mc
        - play_sfx_guard_receive_hit
        - play_sfx_deflect_receive_hit
        - 

- Okay, so it seems like there's a pretty good basis for how the timeline should work.

- [x] FIX: Saving the timeline looks like there's not a proper reset for the model animator after, so fix that.
- [x] FIX: Upon renaming ctrl item labels, reload what they're supposed to be doing (same func as when they're loaded in `serialize()`)
    - The program should crash if the label is not correct upon renaming
    - [x] And creating a new ctrl item.
        - It seems like the data type and stuff is just uninitialized for the creation case so it fails in `get_data_type()`'s unknown switch branch.


## Make custom hit/hurt capsule physics implementation.

- Mmmm I'm doing all this work to make it so that the hitcapsule script can add hitcapsules to a game object.
    - But it feels like this stuff should be controlled by the btafa file, not this.
    - Maybe add ability to create scripts dynamically?
    - Then add btafa file can create these groups dynamically.  <---- This.

```cpp
auto my_transform{ Transform::identity() };
auto my_model{ Model_loader::load_model("SlimeGirl.glb") };
auto my_animator{
    Animator_factory::create_animator_from_template(my_model,
                                                    "SlimeGirl.btanitor") };
auto my_anim_frame_action{
    Anim_frame_action_loader::load_anim_frame_action(my_model,
                                                     my_animator,
                                                     "SlimeGirl.btafa") };
Render_object my_render_obj{ my_transform,
                             Render_layer::VISIBLE,
                             my_model,
                             my_animator };

while (running_game_loop)
{
    // ...

    // Runs at same rate as physics.
    // Updates hitcapsules, events, and variables for simulation that are driven
    // by animation.
    my_anim_frame_action.update();

    // ...
}
```


- [x] Rewrite the `script__dev_anim_editor_tool_state_agent.cpp` to use animator states and templates instead of anim indices.
- [x] I think ImGui for the editor also needs to get reworked bc it's probably using the animator indices too.

- [x] Reincorporate hitcapsule groups.
- [x] Load extra part in btafa and add hitcapsule groups.
    - I think that there needs to be a way to prevent self-collision, so grouping hitcapsule groups together into an entity-owning set of groups would be good.
    - [x] initial loading
    - [x] `deep_clone()`
        - Tried to copy the hitcapsule groups but then copy constructors are deleted.
        - Just defined the copy ctor.
        - Ok, just essentially turned off the `Type const m_type;` in order to reinstate the implicit copy ctor stuff.
    - [x] Use `script_hitcapsule` to get a handle into the bool data handle, and then poll whether the hitcapsules are visible or not based off the labels.
        - Ok I'm doing it a bit different, since I'm deferring the functionality to the animator itself, calling it from the script.
        - [x] Create add/remove script dynamically interface.
        - [x] Use dev anim editor script to dynamically add the script.
            - [x] initial
            - [x] Defer actually mutating the scripts until after the scripts are done running.
                - [x] Adds call.
                - [x] Writes deferring funcs.
        - [x] Write funcs for updating hitcapsule `.enabled` flags
        - [x] Write func for updating hitcapsule transforms.
            - [x] `update_hitcapsule_transform_to_joint_mats()`.
            - [x] Write func `get_joint_matrices_at_frame()`.
    - Okay, at this point everything should be working!!! Now we just need to see it to believe it!

- [x] Add visual representation of when hitcapsules are active.
    - Use `debug_render_job.h` for now.
    - [x] Write funcs to submit the capsules to debug render pool.
    - [x] Write `emplace_debug_line_based_capsule()` with regular `emplace_debug_line()`.
        - [x] Initial
        - It works when I enable/disable on the timeline!!!
        - [x] BUGFIX: Saving timeline doesn't work again...
            - [x] Seems like it works now that the `k_num_lines` is increased? (1024 -> 16384)
                - [x] Added an assert to make sure that the num active line draw jobs (`m_active_indices`) does not exceed the max `k_num_lines`.
            - [x] Maybe anims not switching when the state changes in the gui is a related issue??
                - BUT FIX THIS IMMEDIATELYYYY
        - [x] Shorten the drawing timeout so that it doesn't have a bunch of overlapping draws.
    - [x] Connect a capsule to bone of model to test now.

- [x] Create hitcapsule editor (and group assigner) ImGui.
    > Honestly, just making it so that it edits the positions and radius of the capsules would be fine, instead of having to worry about creating new capsules (bc that all can be done by editing the txt in `.btafa` files).
- [x] It kinda seems like more stuff needs to get checked.
    - Multiply `origin_a` and `origin_b` by the bone matrix and then display those as well (or, just maybe make these editable fields inside imgui too (or just disabled-editable (so... just viewable))).
        - [x] misc, have `calcd_origin_a/b` also get mutated when changing the `origin_a/b` in the imgui editor for capsules that don't recalc their `calcd_origin_a/b` after initialization.
    - Ok after positioning in the right spot w `0_TPose`, it works perfectly.

- @THOUGHT: I realllllly reaaallllyyyy wish that there was a multi-view editor (1 reg view, 3 ortho axis views) and that would make this a lot easier to work with.

- [x] Allow saving to `.btafa` from gui. Save changes if dirty.
    - [x] Make dirty more strict, where you can't change the anim state while it's dirty
        - [x] Tooltip for that too.

- [ ] ~~Write and use batched version of `emplace_debug_line()` for `emplace_debug_line_based_capsule()` inner func.~~
    - Do this in the future.

- [x] Fix reloading back into imgui anim frame data editor context
    - It's the issue w using static vars everywhere I think.

- [x] Add the hitcapsule group sets into game mode.
    - [ ] ~~Assign `model_animator`.`m_anim_frame_action_controls` with valid controls.~~
    - [ ] ~~Assign `model_animator`.`m_anim_frame_action_data`.`hitcapsule_group_set` with hitcapsule group set.~~
    - [x] (@NOTE: These two vars^^ are taken care of with this func call) Configure animator with `configure_anim_frame_action_controls()`
    - [ ] ~~Have multiple sets interact with each other.~~
        - Do this in the future!

- [x] BUGFIX: For some reason processing script list mutation requests crashes.
    - Answer: capturing `this` in the static lambda actually breaks if something else wants to be used. So I just removed the lambda.

> @FUTURE: Pull "hitcapsule_group_set" out of .btafa file.


## Misc things I want please please please.

- [x] Make simple viewport changing buttons for +-X, +-Y, +-Z positions.
    - This is for making editing the hitcylinders easier.
    - [x] Initial
    - [x] Zoom and move.
        - Right click to click 
        - [x] Basic works.
        - [x] Calc basis vectors correctly.

- [ ] ~~Remove "change viewport with f1" button from main viewport.~~


## Use simple anim dude and make dummy character and create fighting animation movesets.

> Cece suggested naming the dummy character "Frank-e", so I'll name them that.

- [x] Make simple sample anim.
- [x] Create capsules and integrate into the btafa and btanitor systems.

- [x] BUGFIX root bone motion disappeared for some reason.
    - Ummm, ig it was a blender bug? Bc closing and reopening just fixed it.

> I realize that I think the simulation thread of the game needs to hold the master timing. I think that rendering should be its own thing and then the simulation needs to be the sole proprietor of everything. I think having animators move extremely consistently (i.e. on an integer frame tick instead of float increments), ESPECIALLY the ones that have hitcapsules attached to them, is necessary for good swordplay.
>
> So basically, the game runs unhindered on the main thread for simulation. Once two simulations are done running, then a perpetual render job is started, where it has its own `deltatime`, thread, etc. It is started at the beginning of the first simulation (time 0.0), and it should theoretically not catch up to the simulation thread. If that happens, then the render thread should clamp its global `time` to right before the frame it's waiting on.
>
> But then, the issue comes: what hz should this run at? 30? 40? 50? 60? It needs to be a set number to be consistent. To high and it'll be harder to run too.

- ^^The above^^, I don't think I'll worry about it right now. Since there are things like mouse camera controls, mouse clicking and typing (i guess just UI interation stuff?) that I don't want to run inside the simulation thread, I'll need to think about this some more.
    - I think the main point I was trying to make above is that having the hitcapsules be updated inconsistently is sucky. It should be super consistent, and not depend on the render timing of the animator (since it grabs the animator's `m_time` and gets the floored frame and uses that, but it still has to depend on `m_time` which is a render-thread updated variable.).
    - If i were to redo this engine, I would definitely just make everything a fixed timestep and have rendering run right after simulation. Then when I want to separate out the rendering into its own thread, then I would choose how stuff gets divvied up, but for the most part everything is simulator-thread driven and then the render thread lerps the provided values.

>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
>                SCRIPT REFORM
>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
- Basic plan for the script reform:
    ```cpp
    auto& hecs_system{ find_service<HECS_system>() };

    // Adding a struct.
    hecs_system.emplace_struct("Struct_type",   // Struct typename as str.
                               Struct_type{});  // Struct default data.

    // Add a system (kind of like the script but doesn't belong to gameobjects/entities).
    hecs_system.emplace_system("Concrete_system_type",       // System typename as str.
                               std::unique_ptr<System_ifc>(  // Unique instance of system as ifc ptr.
                                   new Concrete_system_type())
                               { "Struct_type",              // List of required structs (checked when entity is added to system).
                                 "Struct_type_2" });

    // Add system ordering (@NOTE: if this is omitted for a struct, then it's a free for all for
    // every system using the struct; that would be the ideal for structs that aren't
    // accessed/written multiple times in an entity).
    hecs_system.add_struct_system_order("Struct_type_69",                  // Struct typename.
                                        { { "Concrete_system_type_222" },  // All system typenames that use `Struct_type_69`, placed in an order.
                                          { "Concrete_system_type_555",
                                            "Concrete_system_type_666" } });  // @NOTE: both `555` and `666` can be run at the same time, but if an entity is added to both `555` and `666` then an error occurs.

    // Creating an entity.
    hecs_system.create_entity("Entity_name",             // Entity name.
                              { "Struct_type",           // Structs allocated for new entity.
                                "Struct_type_2",
                                "Struct_type_3" },
    //                          { "Concrete_system_type",  // Systems to add entity to.
    //                            "Concrete_system_type_2" }
                              );  // ^^No more adding systems. I'm gonna try to go thru with ECS.

    // Querying for an entity.
    Concrete_system::invoke_system() override
    {
        static auto const k_query{
            component::System_helpers::compile_query_string(
                "(Struct_type_2 && Struct_type_3 && Struct_player && !Struct_deku_scrub)") };
        auto comp_lists{ service_finder::find_service<component::Registry>()
                             .query_component_lists(k_query) };
    }
    ```

- [ ] Emplace everything into a `string->size_t` for the struct name for deserialization.
- [ ] Attach structs ~~and scripts~~ to gameobject
    - [ ] When gameobjects are added to the script execution pool,




- [ ] 
- [ ] Create script for character controller movement but without the input from player.
    - [ ] Perhaps this could just be a script that sends its data to another script? Maybe there could be a connections type thing? Mmmmm maybe provide a `on_start()` func for scripts where they could get the script that they need to send to and stuff?
>-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-

- [ ] Add another dummy character that plays the same anim as player.

- [ ] Write the hitcapsule group sets interact w each other.
    > @NOTE: This is kind of the order of checking I think would be good for the interaction.
    - [x] Cancel check if same hitcapsule group set.
    - [ ] Cancel check if hitcapsule group set A does not have any enabled hitcapsule (give hurt).
    - [ ] Cancel check if hitcapsule group set B does not have any enabled hitcapsule (receive hurt).
        - @NOTE: For iframes, the animation should disable the receive-hurt hitcapsule.
        - @REPLY: Or wait, would a different method be better? For debouncing hits, bc that's needed too tho.

    - [ ] Create bounding sphere for both interacting sets (set A's enabled hitcapsule (give hurt), and set B's enabled hitcapsule (receive hurt))
        - For each set of hitcapsules:
            - Gather all `calcd_origin_a` and `calcd_origin_b` and find the midpoint.
            - Find the radius of the bounding sphere by taking each capsule, taking the distance of each origin, adding `radius` to the distance, and then finding the max of the radius-included distances.

    - [ ] Compare the bounding spheres to see if they collide. Cancel if they don't. Continue if they do.

    - [ ] Check each individual hitcapsule in set A with all of the others in set B. `O(N^2)` unfortu (^_^;)
        - For each hitcapsule pair, create their respective bounding sphere, so for each one:
            - Find the midpoint between `calcd_origin_a` and `calcd_origin_b` for the bounding sphere.
            - Find the radius of the bounding sphere with:
                ```cpp
                bounding_sphere_radius = (glm_vec3_distance(capsule.calcd_origin_a,
                                                            capsule.calcd_origin_b)
                                          * 0.5f)
                                         + capsule.radius;
                ```
        - Then if the bounding sphere check succeeds, check the actual real capsule-to-capsule check.
            - @TODO look up a reference for this!!!!!

    - [ ] If the real capsule-to-capsule check succeeds, then `break;` and show that the overlap occurred.
        - This is where the callbacks or event for "got hit" would get triggered or something.
        - Mmmmm maybe the callback would be as the form of both game objects having a `script_hitcapsule_processing` script, both those scripts get searched and found, and then for set A's game obj's script, the "you hit a hitcapsule, deal hurt" func gets called, with set B's game obj's script supplied as a parameter so that inside that "deal hurt" func it calls set B's script's "i got hurt" with all the hurt stats, with set A's script, in case set B's script calls an "i was parrying" call.
            > This interaction could get simplified with data, but maybe having scripts could be good... ahh but having a way to send messages like this is gonna be kinda hard I think. The way described above would work if the `find_script()` func was sophisticated and could find a script attached to the game object that has a certain interface tho.
            >
            > Maybe then "interface groups" would be a useful thing to attach to scripts? Or like a script tag system? And `find_script_by_tag()` or something.

- [ ] REFACTOR: Delete the `calc_orig_pt_distance()` method in hitcapsule bc this info is really only needed when doing the actual collision and isn't needed most of the time.

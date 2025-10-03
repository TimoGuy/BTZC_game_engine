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

- [ ] Add the hitcapsule group sets into game mode.
    - [ ] Have multiple sets interact with each other.

- [ ] Make simple viewport changing buttons for +-X, +-Y, +-Z positions.
    - This is for making editing the hitcylinders easier.

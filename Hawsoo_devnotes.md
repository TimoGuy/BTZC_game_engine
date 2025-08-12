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

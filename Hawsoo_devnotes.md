# Hawsoo's Dev notes.

## Console window implementation.

- There needs to be like a way to retrieve all the data from the logger. Or maybe there could just be a `get_head_and_tail()` func with then just scanning the applicable rows.
- Oh and then a clear logs func.
- For disabling the logger, a message could just get sent "Switching to in-editor console logger" and then `set_logging_print_mask(NONE)` could get called, and then undone when the window is gone.

- Aaaaggghhh it looks like there's not gonna be anymore ansi coloring in the console bc it's kinda haaaarrd ^_^;

- Ok this seems done for now.

- NEVERMIND!!! The logs printed are off.
    - [x] Fixed.

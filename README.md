# BTZC_game_engine
Bozzy-Thea Zelda-like Collectathon Game Engine. Simple to get off the ground.


## Notes.

1. [ ] Get basic renderer assembled.
    - [x] Get everything stubbed out for simple hdr renderer.
    - [ ] Create mesh importer (simple obj meshes for now).
        - [x] Use tinyobjloader to load obj file.
        - [x] Import the mesh into the model-mesh data structre.
        - [ ] Upload mesh data with OpenGL.
        - [x] Write render functions.
        - [ ] ~~Bonus points for obj writer as well.~~
    - [x] Write simple material system.
        - [x] Get ifc and connect it with mesh renderer.
    - [ ] Create simple probuilder-like mesh
    - [ ] Create simple cylinder mesh (representing player character controller).
    - [x] Create render object structure.
    - [ ] Create gameobjects which can use the render object with the mesh.

1. [ ] Get jolt physics working with renderer.
    - [ ] Setup jolt physics world (simple singlethreaded job system for now).
    - [ ] Create bridge between fixed physics timestep and renderer.
        - [ ] Have power to slow down renderer if physics sim is too slow.
    - [ ] Create CharacterVirtual cylinder (exactly like tuned jolt physics example).
    - [ ] Add physics component to gameobjects.

1. [ ] Code review.
    - [ ] Is the gameobject architecture wanted?
    - [ ] Should refactor to put data together in better ways?
        - [ ] Is this important to think about this early and would it create friction at the expense of performance?

1. [ ] Add cascaded shadow maps to renderer.
    - @TODO.

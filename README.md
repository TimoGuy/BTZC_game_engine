# BTZC_game_engine
Bozzy-Thea Zelda-like Collectathon Game Engine. Simple to get off the ground.


## Notes.

1. [x] Get basic renderer assembled.
    - [x] Get everything stubbed out for simple hdr renderer.
    - [x] Create mesh importer (simple obj meshes for now).
        - [x] Use tinyobjloader to load obj file.
        - [x] Import the mesh into the model-mesh data structre.
        - [x] Upload mesh data with OpenGL.
        - [x] Write render functions.
        - [ ] ~~Bonus points for obj writer as well.~~
    - [x] Write simple material system.
        - [x] Get ifc and connect it with mesh renderer.
    - [x] Create simple probuilder-like mesh
    - [x] Create simple cylinder mesh (representing player character controller).
    - [x] Create render object structure.
    - [x] Create gameobjects which can use the render object with the mesh.
    - [x] Write shaders:
        - [x] Basic Geometry color shader.
        - [x] Postprocessing shader w/ ndc quad code.
    - [x] Rework imgui window drawing to make it control the size of the rendering viewport.

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

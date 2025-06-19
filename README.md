Please install a compiler and link it in task.json in order to run this project in vscode.

The main working space is very cluttered and disjointed, but it is essentially setting up OpenGL environment, loading models and materials, currently you can do the following things:
  1. Use Mouse/WASD/Mouse wheel to: Look/Move/Zoom around.
  2. Ctrl+S to take screen shot
  3. Change OBJ models path
  4. Change containing box(classic cornell box, or something more exotic like full mirrors everywhere?)
  5. Change render resolution, cornell light intensity, number of samples and frames for screenshot for high quality offline rendering (WARNING: Due to my bad understanding of resources management in C++, high quality screen shot will most likely take a long time to render and will slow down your computer)

Currently, the project supports loading OBJ files, the .obj (must be consist of only triangulated mesh), it supports basic diffusive, texture, and lightning materials.

To load your favorite model of choice, please open your model in Blender, export as obj files(remember to triangulate mesh and include vertex texture coordinates), make sure the materials and texture are properly configured (a model should be a folder of 1 .obj file, 1 .mtl file and 1 textures folder, the texture name in the .mtl file must match the texture name in the textures folder), finally all models should be stored at the RayTracing/Data folder.

The rendered screenshot are stored at RayTracing/Images/test.png.

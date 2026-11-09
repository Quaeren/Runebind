# RUNEBIND
Welcome to the repo for my game runebind. In runebind you explore different cave systems in search for ruïnes with runes in them. You can combine runes at base camp to make spells to defeat enemies easier and explore faster. 
The game is still in very active development and full of bugs so be were. So far the only systems that have been completed are the generation of the cave map and the height map. 
To try out the cave generation for yourself download the repo and run the CaveGeneration file.
## Testing Cave Generation V2
1. Download the files in this repo, make sure you have all the needed libraries (glm, glew and opengl, fastnoiselite) in your include folder
2. Unzip the files and go into the folder
3. Build the graphics file
   **Linux**
   ```Bash
   g++ -g -std=c++17 graphics.cpp -o graphics -lGLEW -lglfw -lGL
   ./graphics
   ```
## Testing Cave Generation V3
1. Download the files in this repo, make sure you have all the needed libraries (glm, glew and opengl, fastnoiselite) in your include folder
2. Unzip the file and go into the folder
3. Build the CaveGenerationV3 file
   **Linux/MacOS** - You should be able to do the same on Windows
   ```Bash
   g++ -g -std=c++17 CaveGenerationV3.cpp -o graphics -lGLEW -lglfw -lGL
   ./graphics
   ```
   
   
   

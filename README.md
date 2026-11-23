# RUNEBIND
Welcome to the repo for my game runebind. In runebind you explore different cave systems in search for ruïnes with runes in them. You can combine runes at base camp to make spells to defeat enemies easier and explore faster. 
The game is still in very active development and full of bugs so be were. So far the only systems that have been completed are the generation of the cave map and the height map. 
The game is written in c++ and uses Opengl and GLM to render and calculate the voxel layout I choose to use with the game. Voxels where a good choice as you do a lot of customization with them while still keeping a blocky style which I like.
To try out the cave generation for yourself download the repo and run the CaveGeneration file.
## Testing Cave Generation V3
![Image of testing for gen 3 cave generation](images/screenshot-2026-11-09T11:43:30.095262162+01:00.png)
This generation I am momentarily building and testing makes a long corridor which later I will add branching corridors and caverns to. This gives the players a "set path" to explore instead of a large cave like structure which was gen 2. The aim is to also automate the placement of ruins throughout the cave for players to explore.
1. Download the files in this repo, make sure you have all the needed libraries (glm, glew and opengl, fastnoiselite) in your include folder
2. Unzip the file and go into the folder
3. Build the CaveGenerationV3 file
   ```Bash
   g++ -g -std=c++17 CaveGenerationV3.cpp -o graphics -lGLEW -lglfw -lGL
   ./graphics
   ```
## Testing Main Version
![Image of Gen 3 cave generation](images/screenshot-2026-11-16T22:37:24.156574216+01:00.png)
1. Download the files in this repo, make sure you have all the needed libraries (glm, glew and opengl, fastnoiselite) in your include folder
2. Unzip the files and go into the folder
3. Build the graphics file
   ```Bash
   g++ -g -std=c++17 graphics.cpp -o graphics -lGLEW -lglfw -lGL -O2
   ./graphics
   ```
Voxel generation can take upwards of 1 minute or 2 so don't be surprised if nothing shows up at first. If it takes longer then 5 minutes then something is broken somewhere.
After building you are dropped into the cave world as a block, the controls are as follows:
Forward: 'W'
Backward: 'S'
Left: 'A'
Right: 'D'
Jump: 'SPACE'
Change POV: 'V'
Look Around: mouse
change fov/distance from player: scroll wheel
## Further Development
After I finish the cave system I am going to work on getting a player and enemy system up and running. Once that works I'll start work on the main part of the game - the spell crafting. Players should be able to find runes throughout the ruins. Using these runes they can make different combinations and thus different spells.

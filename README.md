# The Nomad
"The Nomad" is a 2D hack-and-slash fighting game where you play as a mercenary in a post-apocalypse world known as Bellatum Terrae.

## Current Build
The currently release version of the game is a Demo Alpha Test, so expect lots of bugs. If you do find a bug, please do me a favor and report it, after all, I can only fix bugs that I know about.

## The Technical Details
Unless you're into programming, this won't really be interesting...

### Quake3e Changes
There are the following changes made to the Quake3e engine that were made in order to extend moddability and modernization:
- Upgraded OpenGL v2.1 renderer to v4.5/6 with Direct State Access, persistently mapped vertex/index/uniform/ssbos(if the hardware allowed it) buffers
- Implemented shader caching for faster load times and more efficient shader compilation
- Replaced the Quake 3 VM with AngelScript & improved AngelScript's vector implementation because it was very memory inefficient (saved roughly half a GB by doing this)
- Replaced Quake 3's sound system with FMOD for easier sound editing and integration, as well as implementing a channel based buffering system akin to the one found in idTech4

### The Future
Since Steam is the place where I intend to publish the game in the future, I'll have to do a rewrite of the engine because it uses GPL v2'd code from the Quake III Arena engine.

### Graphics APIs
__Name__ | __Status__
---------|------------
OpenGL   | Implemented
Vulkan   | Coming soon
D3D11    | In the far future
Metal    | Unless OSX forces you to use it, probably never

Audio: FMod

Everything else that's system specific is either handled by SDL2 or the engine
Future Features: custom simd implementations for standard library functions

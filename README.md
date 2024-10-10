
## Overview 

Particle-Sim is my passion project, a small gravity simulator written in C++,
with SDL for rendering and no physics libraries used. Everything is coded by me.

Currently, its ***really*** not in ***any*** way finished,
and as such there isn't really much to discuss about it.

The goal of this project is to make a simulatir of astrophysical phenomena, 
mainly collisions of bodies, that is:
- fast
- accurate
- small

within reason

## Internals

Here, I'll detail what algorithms I use in the current version.

### Graphics

+ All particles are circles, drawn with a modified version of
bresenhams circle drawing algorithm, that draws filled circles.


### Physics

+ Currently, basic semi-implicit Euler integration is used, as
I have deemed the tradeoff between speed and accuracy of using
more accurate methods is too much.

+ Collisions are handled in a standard way, i.e.
first collecting all colliding pairs, and then resolving each collision
seperately. This is run by default about 10 times, to ensure correct behavior
while maintaining accuracy.

+ The physics mainloop consists of:
  + finding the acceleration due to gravity, of each object
  + updating the velocities of each object
  + finally, updating positions using the updated velocity

## Future Additions

+ Implementing some symplectic intergrator, either leapfrog or verlet
+ Adding a quadtree or grid implementation, to speed up both collisions and gravity
+ Heat
+ A UI

## Limitations

There are several limitations currently:
+ Due to the use of euler integration, and possibly due to some specifics in collision handling,
a collection of bodies that are touching will, *inevitably*, start gaining momentum, and the whole collection
will start spinning faster and faster, until it breaks apart, which is quite unrealistic
+ While care has been put into being able to support lots of bodies simultaniously,
an average laptop could support only up to 700 bodies while still running at 60fps.
+ There is no UI, nor any actual graphics, just circles.
+ The code could be cleaner, and some classes are too bloated.

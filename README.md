Snap-Pad provides an easy, somewhat safe two party OTP capability.

Perhaps it will also provide a solution to brain rot due to excessive datasheet exposure. My fucking eyeballs are burning.

TODO
----

* -- Find lib for NAND bad block management - going to build
* Finalize LED and switch count, color, placement
* Get reflow oven up and running
* Go over use scenarios with Nick F, Hilary M, David H, other cryptonerds
* Look into OpenMoko USB identifiers (can we really use these?)
* Marketing/Spreading the word
  * Bring in John D. to see if KS makes sense
    * How do they handle sales tax? Do they handle sales tax?
  * Large scale: talk to Limor F, Phil T, Nathan S about excess capacity runs.
* Identify a license. (Talk to Alicia S about this, maybe Dustyn R and Addie W)

Random Number Generation
------------------------
* ~~Avalanche (two transistors base-to-base)~~
* Sample an unstable oscillator (w/ LED and thermo to add noise)
  * Ring oscillators are the way to go>
* ~~Zener breakdown~~
* ~~Low-order ADC (slow)~~
* Internal clock

Scheduling
----------
* mid-January: prototype and layout for final RNG design
* mid-January: send off v1 prototypes to OSHPark and barebones
* early February: complete prototype batch, software v0
* end of February: get samples off to appropriate people

blocks: in Chicago end of January

Rough dates
===========
Jan 10: finalize RNG
Jan 13: send off v1 board
Jan 17: assemble barbones prototypes
Jan 22: have hardware in shape prior to Chicago (order v2 there?)


Fast & rough design constraints:
* early boards: test points for programming and debugging
  * I/O and control lines for NAND 
  * VUSB to monitor power line noise
    * We can use the actual edge pads for USB (or put test points under pins)
  


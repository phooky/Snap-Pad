Snap-Pad provides an easy, somewhat safe two party OTP capability.

Perhaps it will also provide a solution to brain rot due to excessive datasheet exposure. My fucking eyeballs are burning.

TODO
----

* <st>Identify 48-pin MSP430F5xx series chip w/ USB module</st> Using MSP430F5508
* <st>Make library part, layout</st> Using TI's library
* <st>Find lib for USB serial HID profile</st> TI has BSD-licensed code
* Find lib for NAND bad block management
* Finalize LED and switch count, color, placement
* Initial board layout
* Get pick-n-place up and running
* Get reflow oven up and running
* Send out v0 to OSH Park/AC
* Go over use scenarios with Nick F, Hilary M, David H, other cryptonerds
* Look into OpenMoko USB identifiers (can we really use these?)
  * If not, how fucked up would it be to buy in to the USB association?
    * Pretty fucked up, best not to support them
* <st>Board layout for on-board USB edge connector</st> Using sparkfun lib
  * <st>Check USB mechanical spec (and anecdotal measurements)</st>
  * Check on board thicknesses available through OSH/AC
* Marketing/Spreading the word
  * Bring in John D. to see if KS makes sense
    * How do they handle sales tax? Do they handle sales tax?
  * Large scale: talk to Limor F, Phil T, Nathan S about excess capacity runs.
* Identify a license. (Talk to Alicia S about this, maybe Dustyn R and Addie W)

Random Number Generation
------------------------
* Avalanche (two transistors base-to-base)
* Sample an unstable oscillator (w/ LED and thermo to add noise)
** Like this option a lot
* Zener breakdown
* Low-order ADC (slow)
* Internal clock

Scheduling
----------
* First board assembled by mid-November
* Working prototype by EOM November
* To Market by mid-December
* (Early run for 3C30 in Hamburg? That's last week in December, and David H's experience with even simple goods in German customs was not encouraging)

Rough dates
===========
* November 15: first assembled board
* November 29: first working prototype
* December 13: first rough batch?
* December 20: announce and launch

Does this schedule pass the smell test? Mmmmaybe. Board design should be fairly straightforward; the code is going to be the biggest issue. How do we get our test cycles down?

Fast & rough design constraints:
* early boards: test points for programming and debugging
  * JTAG test points (NB: MSP430 doesn't do boundry scan, etc.)
  * I/O and control lines for NAND 
  * VUSB to monitor power line noise
    * We can use the actual edge pads for USB (or put test points under pins)
  


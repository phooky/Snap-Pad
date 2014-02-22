# Use Cases for Snap-Pad

## Roles
* The manufacturing tech
* The "twinned" snap-pad user (before snapping)
* The "single" snap-pad user (after snapping)

## Manufacturing cases
#### M1. Assemble boards (solder stencil, pick and place, reflow)
#### M2. Program boards
#### M3. Test boards
#### M4. Pack boards for shipping

## Twinned cases
#### T1. Unboxing twinned SP
#### T2. Randomize OTPs
#### T3. Run diagnosics on twinned SP
#### T4. Factory reset
The snap-pad needs to be reset to fresh from the factory conditions (no random data on the one-time-pads, clear usage maps). Steps:
1. Unplug the twinned snap-pad.
2. Hold down one of the two confirmation buttons and plug the snap-pad into a power or USB port.
3. The two halves of the snap-pad will flash alternately, warning the user of a reset.
4. Release the confirmation button.
5. Press the other confirmation button to confirm the factory reset.
6. While the reset is in progress, the LEDs on both boards will flash in a checkered pattern while the reset is in progress.
7. When the reset is complete, all LEDs will turn off and the snap-pad can be removed.
#### T5. Firmware update
1. Download the firmware update package/software.
2. Unplug the twinned snap-pad.
3. Short the bootloader pins on one side of the snap-pad while plugging it in.
4. Run the update package.
5. When instructed, unplug the snap-pad.
6. Turn the snap-pad around and repeat steps 3-5.
#### T6. Provisioning twinned SPs for an organization
#### T7. Storing twinned SPs

## Single cases
#### U1. Encode 1-4 blocks
#### U2. Decode 1-4 blocks
#### U3. Run diagnostics on SP
#### U4. Read documentation
1. Follow the URL on the reverse of the board.
2. http://snap-pad.info will host tools and documentation for the snap-pad.
#### U5. Genearte random #s
#### U6. Carry or wear SP
#### U7. Wipe SP
#### U8. Rejoin SPs

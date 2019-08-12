# C64-Keyboard-Controlled-Kernal-Switch
This is a Kernal Switch for the Commodore C64, which does not requires any case modifications. The Kernal is selected by holing RESTORE and pushing a number key. 

<img src="https://github.com/svenpetersen1965/C64-Keyboard-Controlled-Kernal-Switch/blob/master/Rev.%200/pictures/2195_-_KA%26KSw.JPG" width="300" alt="Kernal Switch and Kernal Adaptor">

Jusrt holding RESTORE for several seconds will reset the C64. The index of selected Kernal is stored non-volatile, so the last selection will be executed after a power on.

Watch the introduction on youtube: https://youtu.be/pfJxfMYcyzs

The Kernal Switch works in conjuctuion with a Kernal Adaptor:

https://github.com/svenpetersen1965/C64-Kernal-Adaptor-Switch-short-board- for ASSY 250469 Mainboards

https://github.com/svenpetersen1965/C64-Kernal-Adapter-Switch-Long-Board for all other Mainboards

I want to thank MindFlareRetro (https://www.youtube.com/channel/UCBbbiZC2YodIp2Pi-lywG-A/featured) for reviewing my documentation.

# Known Issue
Solved:

Some games/software require an EXROM-Reset, because they change the reset and/or NMI vector. Then the reset does not jump, where it should jump. It even seems, that the problem is persistent even after a power off. Don't panic... your C64 will function again, later. It seems, those games leave some traces in RAM, which have to get lost after being powered off for a while. 

This can be solved by also holding EXROM low while the reset. One of the two reserved IO pins in the pin header can serve to assert EXROM. The software will be changed and the result will be tested.

## Software Rev. 0.1
This software fixes the issue described above. An aditional connection is required. J4, Pin 5 (RS1) has to be connected to the EXROM signal (Expansion port, Pin 9, which can easily be accessed inside the C64) via a 220R resistor. The documentation has been updated.

# REV. 1
Revision 1 is released. The hardware Rev. 1 is tested ok. The documentation is still preliminary and in process of being updarted. It can be refered to the documentation of v0.1, a 220R resistor in the EXROM cable is not required anymore.

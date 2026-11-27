---
title: "(TVC) Rocket"
github: "https://github.com/Pingu207/ROCKET"
description: "A rocket that uses (TVC) and has a onboard ESP32-WROOM-32 flight computer along with sensors on a PCB."
created_at: "2026-08-07"
total_time: "46h"
---

# August 7, 2026
<!-- fabricate:entry 2888 -->

Cleaned up the wiring on my (incomplete) schematic for the PCB which will house the flight computer and sensors

![](https://fabricate.hackclub-assets.com/b92eb145c7605d9b5bd34eea61058075f3a872cea93121607a81028fefa27e26/image-png/Screenshot%202026-08-07%20193302.png)

**Total time spent: 1h 9m**

# August 10, 2026
<!-- fabricate:entry 2887 -->

Fixed naming conventions on net labels and fixed a few wiring issues here and there.

![](https://fabricate.hackclub-assets.com/710ad89d32a5592101dbb3fe7201de1f517fef1ef564ad3d521132e7dabcfb67/image-png/image.png)

**Total time spent: 56m**

# August 10, 2026 (entry 2)
<!-- fabricate:entry 2886 -->

Added more stuff for the schematic(incomplete)

![](https://fabricate.hackclub-assets.com/5dc515f69f42e9a3a1d2fa4c4da77564a9dac4aea721b3beb7ff5c887bdeba1e/image-png/image.png)

**Total time spent: 36m**

# August 11, 2026
<!-- fabricate:entry 2885 -->

Finished wiring the BMP280 as I2C

![](https://fabricate.hackclub-assets.com/00e98bf9dd3df14939c262274549b7928cc871bd1933469a68875630e364b239/image-png/image.png)

**Total time spent: 52m**

# August 12, 2026
<!-- fabricate:entry 2884 -->

Started to build the TVC mount for the rocket

![](https://fabricate.hackclub-assets.com/dd6435218e6228f1de3831d2bda4eb6b6d917bb5d2abf9885421f996d72f66d7/image-png/Screenshot%202026-08-12%20164125.png)

**Total time spent: 22m**

# August 14, 2026
<!-- fabricate:entry 2883 -->

Finished CAD for first servo mount

![](https://fabricate.hackclub-assets.com/2f6e54718d8ecbb8dffb38efa6232c605525365a74e07ccf87814028291f4f90/image-png/image.png)

**Total time spent: 1h 10m**

# August 17, 2026
<!-- fabricate:entry 2882 -->

Started wiring the pyro channels for rocket parachute ejection

![](https://fabricate.hackclub-assets.com/6f63149b4a547ad5073cc028b3b13aa8a8897c7b13dfb9ee4dbb9f4444ef4dc0/image-png/image.png)

**Total time spent: 1h 39m**

# August 19, 2026
<!-- fabricate:entry 2881 -->

Started to make the bottom of TVC

![](https://fabricate.hackclub-assets.com/dae72b23fa07e36ef768ead6fe061fb7882c5f525120b2071e28f332b27d68b9/image-png/Screenshot%202026-08-19%20165559.png)

**Total time spent: 1h 10m**

# August 20, 2026
<!-- fabricate:entry 2880 -->

Today I simulated the rocket's altitude, velocity, and acceleration curves. First, I used regression in Desmos in order to find a fitting polynomial curve for the thrust. After that, I factored in gravity and drag to make an ODE(Riccati differential equation) for the acceleration. Since Desmos is quite slow with numerical integration, I opted to create a python script instead and use matplotlib to graph all the curves, yielding this visual and a calculated apogee of 268.65 meters at 7.66 seconds!

![](https://fabricate.hackclub-assets.com/c325953c16bffbc057f52dd9119b181e7f30f98c86faeffc8cdd391726887e56/image-png/image.png)

**Total time spent: 1h 32m**

# August 23, 2026
<!-- fabricate:entry 2879 -->

I finished the Pyro Firing Channel(parachute ejection circuit) in the schematic, now for the servo connectors and GPS and the schematic is finished!

![](https://fabricate.hackclub-assets.com/6b302f982a4d0e62058e57fedb064b3c5a726127ced6b84cc2cf8e7984012cc4/image-png/image.png)

**Total time spent: 1h 13m**

# August 23, 2026 (entry 2)
<!-- fabricate:entry 2878 -->

Fully finished the TVC! and made some holes in it to save weight.

![](https://fabricate.hackclub-assets.com/fa9c19d9983e3e8d4cc3ae9b908f8398e311bd60674a424604720059004b26b7/image-png/image.png)

**Total time spent: 28m**

# August 24, 2026
<!-- fabricate:entry 2877 -->

Started working on the wiring for the connections to the servos so they can actually do TVC work, chose 20kgcm servos for this, supplied by a 2S LiPo battery, also redid the layout on the schematic(which is close to being finished!)

![](https://fabricate.hackclub-assets.com/5da29a54605c607c61eb49d19f73f145f41eb7548dd88f2a6681ef21cb85bd61/image-png/image.png)

**Total time spent: 3h 7m**

# September 7, 2026
<!-- fabricate:entry 2876 -->

Wsp guys had a break in the middle because school lowk started :( but I am back at it and today I made the body and Nosecone and assembled it on CAD and added it to the TVC as well

![](https://fabricate.hackclub-assets.com/3655db9db529e3fd94ce4d54c65ddb1621dde10e28aeda1ceedc2d4a1568037b/image-png/image.png)

![](https://fabricate.hackclub-assets.com/c054ae68004fa53b0d6039b2032240396b05dda757a92a64468d8bb31cdc997a/image-png/image.png)

**Total time spent: 2h 20m**

# September 11, 2026
<!-- fabricate:entry 2875 -->

Made the fins, made some changes to the inner body, finished the servo power circuit, and started the SD logging circuit.

![](https://fabricate.hackclub-assets.com/672f05498307704545b3652b52b77fab75ee53fe6ed26b1825f5ecefd25e697f/image-png/Screenshot%202026-09-11%20191628.png)

**Total time spent: 3h 29m**

# September 13, 2026
<!-- fabricate:entry 2874 -->

Made a compartment for the PCB to lay in

![](https://fabricate.hackclub-assets.com/647bfce5ed6a8872b00e45668317bb0f89109ac9fe54cca0cddc97fa4a703375/image-png/image.png)

**Total time spent: 1h 15m**

# September 15, 2026
<!-- fabricate:entry 2873 -->

Finished the schematic, learned a lot in the process, ESP32, MicroSD, ICM42688, BMP280, Power, GND, USBC, UART(conversion), Pyro Firing, and Servo circuits are complete, after a short review period will be ready for quick layout and manufacturer sendoff!

![](https://fabricate.hackclub-assets.com/2c39f7f070e5cfc8ca8ff0a601498c64d2e35d69fc41ca942ef5dc81e6569f9a/image-png/image.png)

**Total time spent: 2h 33m**

# September 21, 2026
<!-- fabricate:entry 2872 -->

Today, I rebuilt the TVC because the last TVC had major issues where the pitch and yaw were not moving correctly, so I redid the whole thing, where this is the bottom part of the TVC. Also fixed some errors on the eeschema including issues with polarity, footprints, component values, power reg, and logic, the continuation of this should ensure that the PCB routing process remains easy.

![](https://fabricate.hackclub-assets.com/cca7f04f8ee6902c3bbb15fed632fa37e049611341859eb26616ec7fc7e47e85/image-png/image.png)

![](https://fabricate.hackclub-assets.com/fc79cacd605a36399a3270866e826c48443451860cbaee0445bc1cced84bde60/image-png/image%20%282.png).png)

**Total time spent: 5h 55m**

# September 24, 2026
<!-- fabricate:entry 2871 -->

Finished V2 of the TVC with all of the errors fixed and it looks more fun tbh but I anyways I did optimize it for the new servos we are using and also fixed the pitch and yaw which were kind of goofy on the last one. But thankfully, this one has no errors and might start to print it in some time.

![](https://fabricate.hackclub-assets.com/407b05e5efb216acbb7d8265d0b8ad6b916ea9e88482ce0bab1cd879e48033a9/image-png/image.png)

**Total time spent: 2h 12m**

# September 26, 2026
<!-- fabricate:entry 2870 -->

Today I worked on fixing minor issues, as a comment on the last post said, and added the rod holes for the servo rods to sit in, and also made some small fixes to the schematic but I was more focused on TVC today.

![](https://fabricate.hackclub-assets.com/9aa93319d8eb739e9c546136d65d8be3209e697f9faa6adc7ac760844d345ce6/image-png/image.png)

![](https://fabricate.hackclub-assets.com/c8e8a5432e73a6e7c17a1c3b49a0c70e22d4291ef9cf893d61676bbba629a792/image-png/image.png)

**Total time spent: 2h 32m**

# October 11, 2026
<!-- fabricate:entry 2869 -->

Added the power from the schematic onto the PCB CAD

![](https://fabricate.hackclub-assets.com/ad225649766458c14e644118b54296063adbd7737969ae51b9b04e6dbd224353/image-png/image.png)

**Total time spent: 20m**

# October 12, 2026
<!-- fabricate:entry 2868 -->

Made some tweaks to the TVC and make the mount to fit ID of the body and maybe this week we will have everything done with the electronics and hardware and will start to order parts!!! And also I have not been made a devlog in sometime because school is lowk cooked but will keep it updated and hopefully have a ROCKETTTTTT bueeee.

![](https://fabricate.hackclub-assets.com/e5dc2b9a46c25789bce2c9def58673b41546364cef8e83c9bcd72a085c513153/image-jpeg/IMG_0968.jpeg)

![](https://fabricate.hackclub-assets.com/3002bd3f77148e6e5d951dcd6a359744b62e5fdfe764f4b18500c5d5ca106e1f/image-png/image%20%283.png).png)

**Total time spent: 1h 34m**

# October 16, 2026
<!-- fabricate:entry 2867 -->

Created footprints for each part, got started routing

![](https://fabricate.hackclub-assets.com/bee270b351f44ae70ec3b163deab22d1167eecfc1155c62c2157767cdbf48827/image-png/image.png)

**Total time spent: 5h 56m**

# October 25, 2026
<!-- fabricate:entry 2866 -->

Last Devlog!! Finished the PCB and the CAD, and am ready to ship(!!)

![](https://fabricate.hackclub-assets.com/48c5e0bc973e6ce230df99cdc6ab8173601a3613ae0c132a4933de166630ab49/image-png/image.png)

![](https://fabricate.hackclub-assets.com/7eaa1d067ff2adbaa5865e668e8a8b8df094a55acac5905604552419bd17d8ba/image-png/image.png)

**Total time spent: 3h 40m**

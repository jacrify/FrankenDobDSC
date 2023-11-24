# The FrankenDob
## A 6" Dobsonian on a dual axis Equatorial Platform with integrated Digital Setting Circles

Interesting stuff that it does:
- Micro controller (ESP32) controlled speed with a stepper motor, to cancel out tangent error
- Digital Setting Circles (encoders) on the alt/azi axis of my dobsonian are integrated with the platform, so I can always see on my ipad where I'm pointing in the sky
- A new method of attaching the top of the platform to the drive system
- A separate base with rotation capability, to allow for easy leveling and polar alignment of the platform whilst in use (rather than kicking it)
- An autofocuser with hand controller, which also controls the platform axis
- ASCOM/Alpaca api for the focuser and platform, to allow synching/postion tracking/pulseguiding
- A "Zlomotion" style fine adjustment system for easily finding targets, with a really neat alt axis clutchable lead scren mechanism 

# Why did you do this? Justify your actions please

Around 2018 I rescued an 6" Dobsonian telescope from an abusive environment. The scope has been stored outside for some period of time, but structurally was pretty sound. It was my first telescope and I fond it a joy to use.

However I quickly found the limitations of my purchase. I live on the (bortle 5) southern edges of Sydney: planetary and luna observing was amazing, but I wanted to see the wonders of faint deep space wispies: nebulae and galaxies. From my observing location, with my scope, this proved challenging.

If I zoomed in on an object too far, it drifted out of view. If an object was too faint, I couldn't find it, and when I could find it it was too faint to see. Much frustration ensued. 

Sometime around 2020 I started building an equatorial platform to address drifting. I also started looking at digital setting circles to help me understand where I am looking in the sky. And, after I purchased the cheapest dedicated astro camera I could buy (the ZWO ASI224MC), I decided I needed an electronic focuser and a ZloMotion style fine adjustment system.

With lots of time, design, and a lot of 3d printing, my Frankendob was born. I'll break down the various components below.

# Wait. What's an Equatorial Platform?

An equatorial platform is a platform that cancels out the rotation of the earth for some period of time. This is slightly harder than it sounds: mechanically you need a platform that rotates around a pivot line that points to the North celestial pole (or South celestial pole if you are in the southern hemisphere). Any telescope sitting on such a platform remains stationary relative to the sky, for a period of time, but only if the platform is accurately aligned, level, and rotates at the exact speed the earth is turning at.

My platform is designed for my latitude (~33.8S): the supports the roller run on parts are 3d printed specifically for this latitude. Working out the shapes of those curves was the hardest part of the design: my Fusion 360 design began with an overly complex 3d sketch mapping out the shape of the curves, and then I built the rest of the platform around it.

# And what's a "dual axis"?

A single axis platform is fine for visual observing (to keep an object from drifting out of view). However if you trying to do any form of astrophotography, you start trying to maximise the 

![Component Diagram](docs/diagrams/out/ComponentDiagram.png)
[Equatorial Platform Code](https://github.com/jacrify/FrankenDobEquatorialPlatform)

[Auto Focuser Code](https://github.com/jacrify/FrankenDobFocuser)

[Digital Setting Circles Code](https://github.com/jacrify/FrankenDobDSC)


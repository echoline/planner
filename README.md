a simple gtk planner

![http://i.imgur.com/BneNbcE.png](http://i.imgur.com/BneNbcE.png)

builds on *nix (with gtk3) and on windows (xp with mingw and gtk)

It has come to my attention that a lot of people are STEALING my Public Domain software. Because it is in the Public Domain, I can't say much but my own code and README, this. I've never felt so anonymously loved. I can't remember if I designed this program or my professor did. I want to make a few design principles clear in my loop-back planning of this planner. First of all, I was going for a synthesis of good UI design and also good "unix" design. In the old days of unix, each user got a ~/.plan file. This app makes a well-organized ~/.plans/ directory for other unix programs. The good UI design is that the computer knows you want to save if you typed it, and it's overall very simple. I'm sorry I learned GTK over other options at this point in my life, but I'm sure if you trust GTK and read my code you will find it pleasantly auditable. The transition of unix to the world of graphics has been rough, and there is room for improvement in the directions of many programming schools of thought. In Linux, I've had it copy over the day's index.txt to ~/.plan, and read it with espeak at progressing volumes throughout the morning as a cue for getting out of bed and typing the command to brew coffee.

Hallowed be the Ori

screenshot of Plan 9 version:

![https://i.imgur.com/5cKMRGW.png](https://i.imgur.com/5cKMRGW.png)

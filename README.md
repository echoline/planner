a simple gtk planner

builds on *nix (with gtk3) and on windows (with mingw and gtk3)

![https://i.imgur.com/6Akzarn.png](https://i.imgur.com/6Akzarn.png)

screenshot of Plan 9 version:

![https://i.imgur.com/4y0V2OQ.png](https://i.imgur.com/4y0V2OQ.png)

this program aims to be a synthesis of good UI design and good unix design. it automatically saves every change to a flat-file directory tree laid out like YYYY/MM/DD/index.md, exactly like the werc blagh app, so with file linking or bind mounts you can use it to edit werc blog posts. it doesn't allow the location of the data to be changed over keeping the program simple. you can manage that with the OS on unix or plan 9, and you can manage synchronization with a revision control system. it aims for simplicity over built-in features and plays well with other programs.

a simple gtk planner

builds on *nix (with gtk3) and on windows (with mingw and gtk3)

![https://i.imgur.com/6Akzarn.png](https://i.imgur.com/6Akzarn.png)

screenshot of Plan 9 version:

![https://i.imgur.com/RTzipjS.png](https://i.imgur.com/RTzipjS.png)

this program aims to be a simple synthesis of good UI design and good unix design. it automatically saves every change to a flat-file directory tree laid out like $HOME/.plans/YYYY/MM/DD/index.md on unix and $home/lib/plans/... on Plan 9.

it doesn't allow the location of the data to be changed over the goal of keeping the program simple. you can manage that with the OS on unix or plan 9 with linking or bind mounts.

as another example, you can manage synchronization with a revision control system.
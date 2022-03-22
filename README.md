## TINT2 needs help

I had to adopt tint2 from [o9000](https://gitlab.com/o9000/tint2) account rather automatically after it was suddenly sentenced to end-of-life by previous maintainer without warning. My free time is too limited, so I may not be able to keep pace with all bugs and feature requests, some of whom need to be recovered yet (Chris Lee disabled sections for MR and issues).

There already was number of fixes, optimizations and just enhancements since 2021, just now I just merging whatever I already have. There are still few more ideas to implement, until I can consider tint2's development is exhausted without breaking some of its own core rules, starting with lightweightness and versatility.

Testing for BSD specifically will not be possible as I don't use it anywhere, so at most I'm able to check POSIX compatibility.

So, for short - any help is welcome, not a problem if someone maintains own fork for a while when I'm unable to respond about own fork.

Code does not die. Not as long as there's compiler, able to build it, and system, able to run it, in the world. Moreover while it's FLOSS project.

# Latest stable release: 17.1.1

Changes: https://gitlab.com/nick87720z/tint2/blob/17.1.1/ChangeLog

Documentation: [doc/tint2.md](doc/tint2.md)

Compile it with (after you install the [dependencies](https://gitlab.com/o9000/tint2/wikis/Install#dependencies)):

```
git clone https://gitlab.com/nick87720z/tint2.git
cd tint2
git checkout 17.1.1
mkdir build
cd build
cmake ..
make -j4
```

To install, run (as root):

```
make install
update-icon-caches /usr/local/share/icons/hicolor
update-mime-database /usr/local/share/mime
```

And then you can run the panel `tint2` and the configuration program `tint2conf`.

Please report any problems to https://gitlab.com/nick87720z/tint2/issues. Your feedback is much appreciated.

P.S. GitLab is now the official location of the tint2 project, migrated from Google Code, which is shutting down. In case you are wondering why not GitHub, BitBucket etc., we chose GitLab because it is open source, it is mature and works well, looks cool and has a very nice team.

# What is tint2?

tint2 is a simple panel/taskbar made for modern X window managers. It was specifically made for Openbox but it should also work with other window managers (GNOME, KDE, XFCE etc.). It is based on ttm https://code.google.com/p/ttm/.

# Features

  * Panel with configurable set of applets:
    - taskbar, system tray, clock, launcher
    - arbitrary buttons with configurable commands per each mouse button (including scroll buttons)
    - executors - like buttons, but using configurable command to set appearance (see manual)
  * Easy to customize:
    - Color/transparency on fonts, icons, borders and backgrounds;  
    **Note:** Full transparency requires a compositor such as Compton (if not provided already by the window manager, as in Compiz/Unity, KDE or XFCE);
    - Customizable mouse events;
    - Customizable interpreters for action commands;
  * Pager like capability: move tasks between workspaces (virtual desktops), switch between workspaces;
  * Multi-monitor capability: create a panel for each monitor, showing only the tasks from that monitor;

# Goals

  * Be unintrusive and light (in terms of memory, CPU and aesthetic);
  * Follow the freedesktop.org specifications;
  * Make certain workflows, such as multi-desktop and multi-monitor, easy to use.

# I want it!

  * [Install tint2](https://gitlab.com/o9000/tint2/wikis/Install)

# How do I ...

  * [Install](https://gitlab.com/o9000/tint2/wikis/Install)
  * [Configure](https://gitlab.com/nick87720z/tint2/blob/master/doc/tint2.md)
  * [Add applet not supported by tint2](https://gitlab.com/o9000/tint2/wikis/ThirdPartyApplets)
  * [Other frequently asked questions](https://gitlab.com/o9000/tint2/wikis/FAQ)
  * [Obtain a stack trace when tint2 crashes](https://gitlab.com/o9000/tint2/wikis/Debug)

# Known issues

  * Graphical glitches on Intel graphics cards can be avoided by changing the acceleration method to UXA ([issue 595](https://gitlab.com/o9000/tint2/issues/595))
  * Window managers that do not follow exactly the EWMH specification might not interact well with tint2 ([issue 627](https://gitlab.com/o9000/tint2/issues/627)).
  * tint2-send refresh-execp doesn't work without visible windows (e.g. hidden by autohide option).

## Development issues

  * Chance for deadlock inside libasan if built with ASAN support (gcc bug).
  * Chance for freeze in the tracing instrumentation functions when allocation (malloc, calloc) functions are traced.

# How can I help out?

  * Report bugs and ask questions on the [issue tracker](https://gitlab.com/nick87720z/tint2/issues);
  * Contribute to the development by helping us fix bugs and suggesting new features. Please read the contribution guide: [CONTRIBUTING.md](CONTRIBUTING.md)

# Links
  * Home page: https://gitlab.com/nick87720z/tint2
  * Download latest release: https://gitlab.com/nick87720z/tint2/-/archive/17.1.1/tint2-17.1.1.tar.bz2
  * Git repository: https://gitlab.com/nick87720z/tint2.git
  * Documentation: https://gitlab.com/o9000/tint2/wikis/home
  * Downloads: https://gitlab.com/o9000/tint2-archive/tree/master or https://code.google.com/p/tint2/downloads/list
  * Old project locations (inactive): https://gitlab.com/o9000/tint2 https://code.google.com/p/tint2

# Screenshots

## Default config:

![Screenshot_2016-01-23_14-42-57](https://gitlab.com/nick87720z/tint2/uploads/948fa74eca60864352a033580350b4c3/Screenshot_2016-01-23_14-42-57.png)

## Various configs:

* [Screenshots](https://gitlab.com/o9000/tint2/wikis/screenshots)

## Demos

* [Compact panel, separator, color gradients](https://gitlab.com/o9000/tint2/wikis/whats-new-0.13.0.gif)
* [Executor](https://gitlab.com/o9000/tint2/wikis/whats-new-0.12.4.gif)
* [Mouse over effects](https://gitlab.com/o9000/tint2/wikis/whats-new-0.12.3.gif)
* [Distribute size between taskbars, freespace](https://gitlab.com/o9000/tint2/wikis/whats-new-0.12.gif)

## More

* [Tint2 wiki](https://gitlab.com/o9000/tint2/wikis/Home)

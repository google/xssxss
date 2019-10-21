xssxss
======

Normally, [Chrome](https://www.google.com/chrome/) pauses your screensaver while
you’re watching video. If you use [xmonad](https://xmonad.org/) and
[XScreenSaver](https://www.jwz.org/xscreensaver/) together, though, it won’t.
xssxss fixes this behavior; simply run

    make
    LD_PRELOAD=$PWD/libxssxss.so google-chrome

and ta-da, Chrome will disable your screensaver like it’s supposed to.

Motivation
----------

Chromium disables the screensaver through D-Bus. Neither XScreenSaver nor xmonad
implements a D-Bus interface, so [Chromium falls back][XSSSuspendSet] to calling
[`XScreenSaverSuspend`][Xss(3)]. Despite its name, this is an X library function
and [completely unrelated to XScreenSaver][Ubuntu 718176 comment 3], so
XScreenSaver never picks up on it and remains active.

xssxss (“`XScreenSaver`/XScreenSaver”) translates between X’s `XScreenSaver`
functions and XScreenSaver’s interface by executing `xdg-screensaver reset` when
Chrome calls `XScreenSaverSuspend`.

[Ubuntu 718176 comment 3]: https://bugs.launchpad.net/ubuntu/+source/xscreensaver/+bug/718176/comments/3
[XSSSuspendSet]: https://cs.chromium.org/chromium/src/services/device/wake_lock/power_save_blocker/power_save_blocker_x11.cc?l=225&rcl=2edab67ba89b69c8b6117440faf1b8397a62ace6
[Xss(3)]: https://www.x.org/releases/X11R7.7/doc/man/man3/Xss.3.xhtml

Future work
-----------

Ideally, users would be able to control XScreenSaver using D-Bus. However, I
think adding a D-Bus dependency to XScreenSaver is not the best approach;
instead, someone should write an adapter daemon that implements [a reasonable
D-Bus interface][idle-inhibition-spec] and executes `xscreensaver-command` to
control XScreenSaver.

Once that daemon’s written, Chromium will need to use it. There are multiple
D-Bus interfaces for disabling various screensavers, and Chromium [currently
picks based on the running desktop environment][SelectAPI]. Chromium will need
modification to probe the available interfaces on the bus and select an
appropriate one. There’s a [`TODO` to this effect][SelectAPI TODO], which
suggests the project would be receptive to a patch.

[idle-inhibition-spec]: https://people.freedesktop.org/~hadess/idle-inhibition-spec/re01.html
[SelectAPI]: https://cs.chromium.org/chromium/src/services/device/wake_lock/power_save_blocker/power_save_blocker_x11.cc?l=450&rcl=fdece9dd1e1f7840a643c85e2d89f356b153fe2c
[SelectAPI TODO]: https://cs.chromium.org/chromium/src/services/device/wake_lock/power_save_blocker/power_save_blocker_x11.cc?l=447-448&rcl=999b2f18d8c62250dd19c85f6c2bb370e68fba8f

Disclaimer
----------

This is not an official Google product.

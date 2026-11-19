Empire Earth: Age of AI
=======================

An unofficial fan mod for Empire Earth 1.04 and The Art of Conquest. Not
affiliated with or endorsed by the rights holders of Empire Earth.

Computer players that play fair - they pay full price and see only what they
have scouted - and still build, expand, defend and attack like a player. Plus
fixes for the game itself: windowed and borderless mode, a camera that zooms
out further, selections of more than 63 units, smoother movement and more.

The mod was written largely with AI coding tools.


Installing
----------

Unpack the zip into your Empire Earth folder, next to "Empire Earth.exe".
It adds:

  ageofai.dll          the mod (the game loads it by itself)
  ageofai_config.exe   every setting in one window, with its explanation
  ageofai\             the settings (ageofai.ini), the AI personalities,
                       the log, and this file

Then start Empire Earth as usual.

Works with: the GOG version, the Steam Gold Edition, the Vanilla setup from
empireearth.eu and NeoEE (Empire Earth 1.04), and The Art of Conquest
(EE-AOC.exe). Big maps and armies need 4 GB of memory: ageofai_config.exe,
page Game, button Game exe.


Switching it off
----------------

To play the original game, set enabled=0 at the top of ageofai\ageofai.ini.
To uninstall, delete ageofai.dll, ageofai_config.exe and the ageofai folder.
If you switched on "Settings in a file" in ageofai_config.exe, switch it off
there first - the game won't start without the DLL while it is on.


Multiplayer
-----------

Not ready yet: the main menu's Multiplayer button shows a box and stays in the
menu. To try it anyway, every player needs the same version of the mod and the
same settings, with multiplayer=1 in ageofai.ini - expect problems even then.
Never in NeoEE's ranked games. To play multiplayer without the mod, set
enabled=0.


More
----

Source code, documentation and bug reports:
  https://github.com/Grille/Empire-Earth-Age-of-AI

If something goes wrong, the log ageofai\ageofai_log.csv helps with a report.


License
-------

Copyright (C) 2026 Paul Hirch. Free software under the GNU General Public
License v3.0 (LICENSE.txt in this folder): you may use, share and change it,
and anything you distribute that is built from it must be published under the
same license, with its source code - the source is at the address above.

Empire Earth and its files belong to their rights holders and are not part of
this mod.

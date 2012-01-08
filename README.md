OpenCOMAL
=========

This is a slightly patched version of
[Jos Visser](http://www.josvisser.nl/)'s
[OpenComal](http://www.josvisser.nl/opencomal/)
[0.2.6](http://www.josvisser.nl/opencomal/opencomal-0.2.6.tar.gz)
(stable branch).


Why this fork?
--------------

When trying to run the latest stable or instable OpenComal on more or
less recent versions of Linux (Ubuntu 8.04 and Ubuntu 10.04), the
included Linux binaries `opencomal` and `opencomalrun` immediately
crashed on startup. For me, they only worked on ancient Ubuntu
6.06. Simply re-compiling the sources did not help, so I tracked down
the problem and fixed it in the C sources.

After 3 or 5 years of being unusable on current Linux distributions,
this much improved dialect of the infamous BASIC programming language
has finally been resurrected!


ToDo
----

* `auto` does not work. It may never have worked in the Linux
  version. So you have to enter line numbers manually for now.
* (For old plans, see
  [doc/TODO](https://github.com/pietsch/OpenCOMAL/blob/master/doc/TODO).)


Further Reading
---------------

* [original README](https://github.com/pietsch/OpenCOMAL/blob/master/README.orig)
* [original documentation](https://github.com/pietsch/OpenCOMAL/tree/master/doc)


* * * * *
*Christian Pietsch*

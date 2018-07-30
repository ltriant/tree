## tree

I wanted to pretty print the directory tree, and I didn't have an internet connection to install the tree command that everyone knows and loves, so I wrote my own dumb and very opinionated version for funsies.

And now, even with a fully functioning internet connection and the defacto standard tree command installed, I'm still updating this one for funsies.

## Install

By default, it will install to `/usr/local/bin`:

```
$ make
$ sudo make install
```

But I like to install it to the `bin` directory of my home directory:

```
$ make
$ make install PREFIX=~
```

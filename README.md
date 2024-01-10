# tree

An opinionated tree command, with only the options and defaults that I like.

# But why?

I originally wrote this because I needed/wanted a tree command, but I had no internet connection at my new home yet to install something like GNU tree, so I wrote my own for funsies.

The first version was written in C, which was then re-written in Nim, and has now been re-written again in Go. Who knows which language the next version will be written in...

# Build and Install

Requires the Go toolchain.

```
$ go build
```

Or to build and install directly:

```
$ go install github.com/ltriant/tree
```

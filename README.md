rock_corpus
===========

Repository containing code accompanying the Rock Corpus dataset used in this
corpus study. The program expand6.c, written in C, expands a harmonic analysis
file into a "chord list." The Perl script process-mel5.pl converts a melodic
transcription into a "note list". The Perl script add_timings.pl adds absolute
timing info to a note list or chord list.

Other Perl scripts were used to extract aggregate statistics about from one or
more harmonic analyses or melodic transcriptions. These programs were used to
generate the statistics presented in our 2011 Popular Music paper:

http://theory.esm.rochester.edu/rock_corpus/2011_paper.html

Notes
=====
expand6.c

The program is in C and requires a C compiler. Compile the program like this
(in a Unix window, e.g. the Mac "terminal" window):

```
$ mkdir bin; cc src/expand6.c -o bin/expand6
```

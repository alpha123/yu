What's this?
------------

This directory contains some empty versions of system header files that are 
used to preprocess files without expanding definitions from system headers. 
This is used to debug the big macro data structures without getting things like 
`NULL` expanded to `((void *)0)`.

See Makefile rule debug_templates.

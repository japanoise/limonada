#!/bin/bash
# Autoformats to house style (Linux with long lines)
# "First off, I’d suggest printing out a copy of the GNU coding standards, and
# NOT read it. Burn them, it’s a great symbolic gesture." - Torvalds
# Furthermore, BSD users will be forced to install bash in order to get properly
# formatted code, and pissing them off is surely a net gain c:

for file in *.c *.h
do
    # A few files should be excluded:
    # - arg.h (my puny program has no right to go against the will of 20h)
    # - anything stb (I'd like to keep them untouched so they're easy to update)
    # - hsv.h (this is optimised for size)
    if [[ ! "$file" =~ ^(arg.h$|hsv.h$|stb_) ]] # bashism
    then
	echo "Formatting $file"
	# I have a bad habit of using sepples-style comments; fix 'em
	# This (should) ignore //s that are preceded by a :, i.e. URLs.
	sed -i -e 's|^\([^:]*\)*//\(.*\)$|\1/*\2 */|' "$file"
	# Now actually call indent on the file
	indent -linux -l100 "$file"
	rm "$file"~ # no backups, we have git for this!!
    fi
done

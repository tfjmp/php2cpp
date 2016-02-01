# php2cpp
Extend php2cpp, original at: http://www.mibsoftware.com/php2cpp/

Just a quick and dirty prototype to demonstrate feasibility.

php2cpp from the start does not support all of PHP feature, require hints for correct typing and is probably a bit buggy (I fixed some of the bug I went through). I started to write some new code to reduce the required annotation in a separate Ruby file, ideally could port the whole code to Ruby (the language for quick and dirty) and improve from there.

For long term viable solution, looking into the details of HPHPc (Facebook PHP transpiler) may be the best. However, the code base is quite large and investment/outcome for a simple proof of concept not good.

In example libphp.h represent what needs to be implemented to compile to the target architecture (i.e. need to re-implement all the built in in PHP functions). At the moment not much there (I have another file with actual code).

Variables are not behaving as they would in PHP. Need to implement custom basic types and ideally want to define custom conversion operators (and other) for those types and try to mimic PHP behaviour.

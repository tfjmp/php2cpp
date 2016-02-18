all:
	../prepare.rb example.php _tmp.php
	../php2cpp _tmp.php tmp.cpp Example
	../uncrustify -c ../build/uncrustify/etc/gnu-indent.cfg -f tmp.cpp > response_example.cpp

clean:
	rm -f _tmp.php
	rm -f response_example.cpp

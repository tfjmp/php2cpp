all:
	../prepare.rb example.php _tmp.php
	../php2cpp _tmp.php _tmp.cpp Example
	../uncrustify -c ../build/uncrustify/etc/gnu-indent.cfg -f _tmp.cpp > response_example.cpp
	rm -f _tmp.php
	rm -f _tmp.cpp

clean:
	rm -f _tmp.php
	rm -f response_example.cpp

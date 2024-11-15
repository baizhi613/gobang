.PHONY:gobang
gobang:gobang.cc logger.hpp util.hpp db.hpp online.hpp room.hpp
	g++ -std=c++11 $^ -o $@ -L/usr/lib64/mysql -lmysqlclient -ljsoncpp
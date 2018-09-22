GLOBAL = e:^\code^\Github^\sBase^\
COMPILER = $(GLOBAL)compiler^\
COMPILER_SRC = $(COMPILER)compiler.cc
COMPILER_HEADER= $(COMPILER)parser.hpp $(COMPILER)scanner.hpp $(COMPILER)compiler.h
DB = $(GLOBAL)db^\
DB_SRC = $(DB)cursor.cc $(DB)engine.cc
DB_HEADER = $(DB)cursor.h $(DB)engine.h $(DB)slice.hpp
STORAGE = $(GLOBAL)storage^\
STORAGE_SRC = $(STORAGE)file.cc $(STORAGE)page_manager.cc
STORAGE_HEADER = $(STORAGE)file.h $(STORAGE)file_format.hpp $(STORAGE)mempool.hpp $(STORAGE)page.hpp $(STORAGE)page_manager.h $(STORAGE)page_ref.hpp
UTIL = $(GLOBAL)util^\
UTIL_HEADER = $(UTIL)blob.hpp $(UTIL)dict.hpp $(UTIL)hash.hpp $(UTIL)latch.hpp $(UTIL)random.hpp $(UTIL)reflection.hpp $(UTIL)stack.hpp $(UTIL)time.hpp $(UTIL)utility.hpp
REFLECTION = $(UTIL)reflection.cc

COMMON_OPTION = -I$(GLOBAL) -std=c++11
TEST_OPTION = -I$(GLOBAL) -lgtest -std=c++11

sbase: $(COMPILER)compiler_test.cc $(COMPILER_HEADER) $(COMPILER_SRC) $(REFLECTION) $(DB_HEADER) $(DB_SRC)
	g++ $(COMMON_OPTION) $(COMPILER)compiler_test.cc $(COMPILER_SRC) $(REFLECTION) $(DB_SRC) $(STORAGE_SRC) -o sbase

compiler_test: $(COMPILER)compiler_test.cc $(COMPILER_HEADER) $(COMPILER_SRC) $(REFLECTION) $(DB_HEADER) $(DB_SRC)
	g++ $(COMMON_OPTION) $(COMPILER)compiler_test.cc $(COMPILER_SRC) $(REFLECTION) $(DB_SRC) $(STORAGE_SRC) -o compiler_test

interpreter_test: $(COMPILER)interpreter_test.cc $(COMPILER_HEADER)
	g++ $(COMMON_OPTION) $(COMPILER)interpreter_test.cc -o interpreter_test

engine_test: $(DB)engine_test.cc $(DB_SRC) $(DB_HEADER) $(STORAGE_HEADER) $(STORAGE_SRC) $(UTIL_HEADER) $(REFLECTION)
	g++ $(TEST_OPTION) $(DB)engine_test.cc $(DB_SRC) $(STORAGE_SRC) $(REFLECTION) -o engine_test -std=c++11

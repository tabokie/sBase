GLOBAL = e:^\code^\Github^\sBase^\
DB = $(GLOBAL)db^\
DB_SRC = $(DB)cursor.cc $(DB)engine.cc
DB_HEADER = $(DB)cursor.h $(DB)engine.h $(DB)slice.hpp
STORAGE = $(GLOBAL)storage^\
STORAGE_SRC = $(STORAGE)file.cc $(STORAGE)page_manager.cc
STORAGE_HEADER = $(STORAGE)file.h $(STORAGE)file_format.hpp $(STORAGE)mempool.hpp $(STORAGE)page.hpp $(STORAGE)page_manager.h $(STORAGE)page_ref.hpp
UTIL = $(GLOBAL)util^\
UTIL_HEADER = $(UTIL)blob.hpp $(UTIL)dict.hpp $(UTIL)hash.hpp $(UTIL)latch.hpp $(UTIL)random.hpp $(UTIL)reflection.hpp $(UTIL)stack.hpp $(UTIL)time.hpp $(UTIL)utility.hpp
REFLECTION = $(UTIL)reflection.cc

engine_test: $(DB)engine_test.cc $(DB_SRC) $(DB_HEADER) $(STORAGE_HEADER) $(STORAGE_SRC) $(UTIL_HEADER) $(REFLECTION)
	g++ -I$(GLOBAL) -lgtest $(DB)engine_test.cc $(DB_SRC) $(STORAGE_SRC) $(REFLECTION) -o engine_test -std=c++11
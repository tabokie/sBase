SOURCE_FILE = *.cc
HEADER_FILE = *.hpp, *.h


test: $(SOURCE_FILE)
	g++ 
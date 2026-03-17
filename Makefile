.PHONY: all lib test clean

src_dir := ./src
include_dir := ./include
bin_dir := ./bin
test_dir := ./test

lib_target := $(bin_dir)/libhbutils.a
test_exe := $(bin_dir)/test

impl_src := \
	checksum.cpp \
	cidr_set.cpp \
	route_table.cpp \
	tb_rate_limiter.cpp
impl_src := $(addprefix $(src_dir)/, $(impl_src))
impl_obj := $(impl_src:.cpp=.o)

test_src := \
	test_checksum.cpp \
	test_cidr_set.cpp \
	test_route_table.cpp \
	test_tb_rate_limiter.cpp
test_src := $(addprefix $(test_dir)/, $(test_src))
test_obj := $(test_src:.cpp=.o)

CXXFLAGS := \
	--std=c++17 -Werror -Wfatal-errors \
	-O1 \
	-I $(include_dir)

all: lib test

lib: $(lib_target)

$(lib_target): $(impl_obj)
	$(RM) $@
	@mkdir -p $(bin_dir)
	$(AR) -rs $@ $^

test: $(test_exe)
	$^

$(test_exe): $(test_obj) $(lib_target)
	g++ -o $@ $^ \
		-l gtest -l gtest_main

$(impl_obj): %.o: %.cpp
	g++ $(CXXFLAGS) -c -o $@ $^

$(test_obj): %.o: %.cpp
	g++ $(CXXFLAGS) -D GTEST -c -o $@ $^

clean:
	$(RM) $(test_obj) $(impl_obj) $(test_exe) $(lib_target)

all: protoc-gen-ruby

clean:
	-rm -f string-utils protoc-gen-ruby

string-utils: string_utils.cc
	-rm -f string-utils
	g++ -std=c++11 -c -I /usr/local/include $^ -o $@

protoc-gen-ruby: string-utils protoc_gen_ruby.cc ruby_code_generator.cc
	-rm -f protoc-gen-ruby
	g++ -std=c++11 -I . -I /usr/local/include $^ `pkg-config --cflags --libs protobuf` -lprotoc -o $@

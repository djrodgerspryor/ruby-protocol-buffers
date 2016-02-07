require 'pathname'

src_dir = Pathname.new 'lib/protocol_buffers/compiler'
out_dir = Pathname.new 'bin'
out = out_dir.join('protoc-gen-ruby')
string_utils_out = out_dir.join('c-string-utils')

directory out_dir.to_s

file string_utils_out.to_s => [src_dir.join('string_utils.cc')].map { |src| src.to_s } do |task|
    sh "g++ -std=c++11 -c -I #{src_dir.to_s} -I /usr/local/include #{task.prerequisites.join(' ')} -o #{string_utils_out.to_s}"
end

file out.to_s => [string_utils_out, src_dir.join('protoc_gen_ruby.cc'), src_dir.join('ruby_code_generator.cc')].map { |src| src.to_s } do
    sh "g++ -std=c++11 -c -I #{src_dir.to_s} -I /usr/local/include #{src_dir.join('string_utils.cc').to_s} `pkg-config --cflags --libs protobuf` -o #{out.to_s}"
end

task :default => out.to_s

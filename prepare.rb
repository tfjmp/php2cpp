#!/usr/bin/env ruby

abort "Usage: ruby prepare.rb [target-file] [output-file]" unless !ARGV[0].nil? && !ARGV[1].nil?

src = File.read ARGV[0]

puts "Preparing object creation..."
src.scan(/\$\w+\s*=\s*new\s\w+;/).each do |target|
  name = /(\$\w+)\s*=\s*new\s(\w+);/.match(target)[1]
  type = /(\$\w+)\s*=\s*new\s(\w+);/.match(target)[2]
  src = src.gsub(target, "#{type} #{name}; /*replace #{target}*/")
end
f = File.new(ARGV[1], "w+")
f.write src

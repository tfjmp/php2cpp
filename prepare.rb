#!/usr/bin/env ruby

abort "Usage: prepare.rb [target-file] [output-file]" unless !ARGV[0].nil? && !ARGV[1].nil?

src = File.read ARGV[0]

puts "Going through file #{ARGV[0]}"

puts "Going through required files..."
src.scan(/(require|include|require_once)\s('|")([\w\/_.-]*)('|")/).each do |file|
  puts file[2]
  abort "prepare.rb failed" unless system "../php2cpp/prepare.rb ./#{file[2]} ./#{file[2]}"
end

puts "Preparing object creation..."
src.scan(/\$\w+\s*=\s*new\s\w+;/).each do |target|
  name = /(\$\w+)\s*=\s*new\s(\w+);/.match(target)[1]
  type = /(\$\w+)\s*=\s*new\s(\w+);/.match(target)[2]
  src = src.gsub(target, "#{type} #{name}; /*replace #{target}*/")
end

puts "Dealing with implements..."
src.scan(/\simplements\s/).each do |implements|
  src = src.gsub(implements, ": public ")
end

puts "Dealing with interface..."
src.scan(/interface\s/).each do |implements|
  src = src.gsub(implements, "class ")
end

puts "Dealing with interface..."
src.scan(/null/).each do |n|
  src = src.gsub(n, "NULL ")
end

f = File.new(ARGV[1], "w+")
f.write src

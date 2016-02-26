#!/usr/bin/env ruby

$visited = Array.new

def prepare src, directory
  puts "Going through required files..."
  src.scan(/(?:require_once|require|include)\s*\(*['|"][\w\/_.-]*['|"]\)*;*/).each do |req|
    inc=directory
    if match = req.match(/(?:require_once|require|include)\s*\(*['|"]([\w\/_.-]*)['|"]\)*;*/)
      file = match.captures
      inc += file[0]
    end
    puts req+" <= "+inc
    if req.include? 'require_once'
      abort 'Broke require_once constraint.' unless !$visited.include? inc
    end
    $visited.push inc
    tmp = File.read inc
    tmp = prepare tmp, directory
    src = src.gsub(req, "?>"+tmp+"<?php")
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

  puts "Dealing null -> NULL..."
  src.scan(/null/).each do |n|
    src = src.gsub(n, "NULL ")
  end
  src = src.gsub("\n", "")
  return src
end


abort "Usage: prepare.rb [target-file] [output-file]" unless !ARGV[0].nil? && !ARGV[1].nil?

puts "Going through file #{ARGV[0]}"
src = File.read ARGV[0]
directory = ARGV[0].gsub(/([\w|-]*.php)/, "")

src = prepare src, directory

f = File.new(ARGV[1], "w+")
f.write src

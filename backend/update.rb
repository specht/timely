require 'date'
require 'fileutils'

timestamp = Date.today().to_s()
puts "Updating #{timestamp}..."

FileUtils::mkpath('timely')

system("ruby fetch.rb")
system("ruby read.rb ./fetch/timely-#{timestamp}.yaml > ./timely/timely-#{timestamp}.txt")

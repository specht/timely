require 'date'
require 'fileutils'

timestamp = Date.today().to_s()
puts "Updating #{timestamp}..."

FileUtils::mkpath('timely')

system("ruby1.9.1 fetch.rb")
system("ruby1.9.1 read.rb ./fetch/timely-#{timestamp}.yaml > ./timely/timely-#{timestamp}.txt")
FileUtils::rm_f("./fetch/timely-#{timestamp}.yaml")
system("gzip ./timely/timely-#{timestamp}.txt")


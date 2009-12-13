require 'yaml'
require 'rexml/text.rb'
require 'fileutils'
require 'date'


def getYearWikiText(year)
	text = `curl http://en.wikipedia.org/wiki/Special:Export/#{year}`
	startIndex = text.index('<text')
	text = text[startIndex, text.size - startIndex]
	startIndex = text.index('>') + 1
	text = text[startIndex, text.size - startIndex]
	text = text[0, text.rindex('</text>')]
	return REXML::Text::unnormalize(text)
end


FileUtils::mkpath('fetch')

timestamp = Date.today().to_s()

File::open("fetch/timely-#{timestamp}.yaml", 'w') do |file|
    (1..2009).each do |year|
        h = Hash.new
        h[year] = getYearWikiText(year)
        file.puts h.to_yaml
    end
end

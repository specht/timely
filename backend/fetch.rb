require 'yaml'
require 'rexml/text.rb'
require 'fileutils'
require 'date'
require 'open-uri'
require 'uri'
require 'set'


$seenUris = Set.new

def getYearWikiText(year)
    if $seenUris.include?(year)
        return nil, nil
    end
	#text = `curl http://en.wikipedia.org/wiki/Special:Export/#{year}`
    text = ''
    open("http://en.wikipedia.org/wiki/Special:Export/#{year}", 'r') do |f|
        text = f.read
    end
	startIndex = text.index('<text')
	text = text[startIndex, text.size - startIndex]
	startIndex = text.index('>') + 1
	text = text[startIndex, text.size - startIndex]
	text = text[0, text.rindex('</text>')]
	result = REXML::Text::unnormalize(text)
    if result.index('#REDIRECT') == 0
        uri = URI.escape(result.sub('#REDIRECT', '').gsub('[', '').gsub(']', '').strip.gsub(' ', '_'))
        return getYearWikiText(uri), uri
    end
    return result, year
end

FileUtils::mkpath('fetch')

timestamp = Date.today().jd()
$file = nil

def fetch(uri)
    content, uri = getYearWikiText(uri)
    if content
        h = Hash.new
        h[uri] = content
        unless $seenUris.include?(uri)
            $file.puts h.to_yaml
        end
        $seenUris << uri
    end
end

File::open("fetch/timely-#{timestamp}.yaml", 'w') do |$file|
    # fetch AD years
    (1..2059).each do |year|
        fetch("#{year}")
    end
    # fetch BC years
    (1..1000).each do |year|
        fetch("#{year}_BC")
    end
    # fetch AD decades
    (1..211).each do |year|
        fetch("#{year * 10}s")
    end
    # fetch BC decades
    (1..169).each do |year|
        fetch("#{year * 10}s_BC")
    end
    # fetch AD centuries
    (1..40).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1
        ending = 'nd' if (year % 10) == 2
        ending = 'rd' if (year % 10) == 3
        fetch("#{year}#{ending}_century")
    end
    # fetch BC centuries
    (1..40).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1
        ending = 'nd' if (year % 10) == 2
        ending = 'rd' if (year % 10) == 3
        fetch("#{year}#{ending}_century_BC")
    end
    # fetch AD millenia
    (1..11).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1
        ending = 'nd' if (year % 10) == 2
        ending = 'rd' if (year % 10) == 3
        fetch("#{year}#{ending}_millenium")
    end
    # fetch BC millenia
    (1..11).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1
        ending = 'nd' if (year % 10) == 2
        ending = 'rd' if (year % 10) == 3
        fetch("#{year}#{ending}_millenium_BC")
    end
end

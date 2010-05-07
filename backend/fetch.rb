require 'yaml'
require 'rexml/text.rb'
require 'fileutils'
require 'date'
require 'open-uri'
require 'uri'
require 'set'

$seenUris = Set.new

def getYearWikiText(year)
    puts year
    if $seenUris.include?(year)
        return nil, nil
    end
    text = ''
    tryCount = 0
    while tryCount < 10
        tryCount += 1
        begin
            uri = "http://en.wikipedia.org/wiki/Special:Export/#{year}"
            open(uri, 'r') do |f|
                text = f.read
            end
        rescue
            next
        end
        break unless text.empty?
    end
    return nil, nil if text.empty?
	startIndex = text.index('<text')
	return nil, nil unless startIndex >= 0
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

$file = nil

File::open("fetch/timely-#{timestamp}.yaml", 'w') do |file|
    $file = file
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
        ending = 'st' if (year % 10) == 1 && year != 11
        ending = 'nd' if (year % 10) == 2 && year != 12
        ending = 'rd' if (year % 10) == 3 && year != 13
        fetch("#{year}#{ending}_century")
    end
    # fetch BC centuries
    (1..40).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1 && year != 11
        ending = 'nd' if (year % 10) == 2 && year != 12
        ending = 'rd' if (year % 10) == 3 && year != 13
        fetch("#{year}#{ending}_century_BC")
    end
    # fetch AD millenia
    (1..11).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1 && year != 11
        ending = 'nd' if (year % 10) == 2 && year != 12
        ending = 'rd' if (year % 10) == 3 && year != 13
        fetch("#{year}#{ending}_millennium")
    end
    # fetch BC millenia
    (1..11).each do |year|
        ending = 'th'
        ending = 'st' if (year % 10) == 1 && year != 11
        ending = 'nd' if (year % 10) == 2 && year != 12
        ending = 'rd' if (year % 10) == 3 && year != 13
        fetch("#{year}#{ending}_millennium_BC")
    end
end

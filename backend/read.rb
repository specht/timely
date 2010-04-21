require 'yaml'
require 'set'
require 'cgi'
require 'rubygems'
require 'wikitext'
require 'date'

# each result line is an entry:
# [jd] [date scope] [marker] [type] [description]
# jd is the time expressed as julian date
# scope is the time resolution and shows how much of the date is certain: 
# 0: nothing, 1: Y, 2: Y/M, 3: Y/M/D
# type is one of [ebd]: event, birth, death
# description is the HTML-formatted, externally linked description


$gk_Parser = Wikitext::Parser.new
$gk_Parser.internal_link_prefix = 'http://en.wikipedia.org/wiki/'

$gk_HeaderSet = Set.new


def parseMonthAndDay(s)
	originals = s.dup
	months = ['january', 'february', 'march', 'april', 'may', 'june',
	'july', 'august', 'september', 'october', 'november', 'december']
	s.strip!
	if (s.downcase.index('unknown') == 0)
		return nil, nil, s
	end
	
	if s[0, 2] == '[['
		s.sub!('[[', '')
		s.sub!(']]', '')
		s.strip!
	end
	
	month = nil
	months.each do |x|
		if (s.downcase.index(x) == 0)
			month = months.index(x) + 1
			s = s[x.size, s.size - x.size]
			s.strip!
		end
	end
	
	unless month
		return nil, nil, originals
	end
	
	day = s.to_i
	if (day == 0)
		day = nil
	end
	s.sub!(day.to_s, '')
	s.strip!
	
	return month, day, s
end


def filterEvents(inYear, text)
    year = inYear.dup
    isBC = false
    if (year.upcase.include?('BC'))
        isBC = true
        year.sub!('BC', '')
        year.strip!
    end
    
    if year.class == String
        if year.include?('millennium')
            year = /\d+/.match(year)[0].to_i * 1000 
        elsif year.include?('century')
            year = /\d+/.match(year)[0].to_i * 100
        elsif year =~ /\d+s/
            year = /\d+/.match(year)[0].to_i * 10
        else
            year = year.to_i
        end
    end
    
    year = -year if isBC
    
    return if year < -4712
    
	dashes = ['-', '&ndash;', '&mdash;', '&#x2013;', '&#x2014;', '&#x2212;', '&nbsp;']
	type = nil
	seenTypes = Set.new
	previousDate = nil
	headerSet = Set.new
	headerStack = Array.new
	headerPath = ''
    if text.class != String
        return 
    end
	text.each_line do |line|
		line.strip!
		if line =~ /^[=]+[^=]+[=]+$/
			headerDepth = 0
			lineCopy = line.dup
			while (lineCopy[0, 1] == '=')
				headerDepth += 1
				lineCopy.slice!(0, 1)
			end
			while (headerStack.size < headerDepth - 1)
				headerStack.push('')
			end
			headerStack = headerStack[0, headerDepth - 1]
			headerPart = line.gsub('=', '').downcase
			headerPart.gsub!('[', '')
			headerPart.gsub!(']', '')
			headerPart.gsub!('{{pagename}}', '')
			headerPart.gsub!(/\d+/, '')
			headerPart.strip!
			headerStack.push(headerPart)
# 			puts "#{line}, #{headerStack.join('/')}"
			headerPath = headerStack.join('/')
			# headerPath is something like /events/by place/asia
			$gk_HeaderSet.add(headerPath)
			# it's a heading
			if line =~ /^==[^=]+==$/
# 				puts "== HEADING #{line}"
				# it's a == heading
				if line =~ /^==\s*.*events.*\s*==$/i
					type = 'event'
					seenTypes << type
				elsif  line =~ /^==\s*births\s*==$/i
					type = 'birth'
					seenTypes << type
				elsif  line =~ /^==\s*deaths\s*==$/i
					type = 'death'
					seenTypes << type
				else
					type = nil
				end
			end
		else
			if type != nil
				line.strip!
				# remove [external links]
				line.gsub!(/([^\[])(\[[^\]]*\])([^\]])/, '\1\3')
				content = nil
				date = nil
				unless line.empty?
					if line[0, 2] == '**'
						# just use the previous date
						line = line.sub('**', '').strip
						date = previousDate
						content = $gk_Parser.parse(line)
					elsif line[0, 1] == '*'
						line = line.sub('*', '').strip
						month, day, line = parseMonthAndDay(line)
						repeat = true
						while repeat
							repeat = false
							dashes.each do |dash|
								if (line.index(dash) == 0)
									line.sub!(dash, '')
									line.strip!
									repeat = true
									break
								end
							end
						end
						month = sprintf('%02d', month) if month
						month ||= '01'
						day = sprintf('%02d', day) if day
						day ||= '01'
						date = "#{sprintf('%04d', year)}-#{month}-#{day}"
						previousDate = date
						content = $gk_Parser.parse(line)
					end
				end
				if content
					content.strip! 
					unless content.empty?
						dateOk = true
						begin
							jd = Date.parse(date).jd()
						rescue StandardError => e
# 							puts "ERROR parsing #{date}."
# 							puts e
							dateOk = false
						end
						
						if dateOk
							content.gsub!(/&lt;\!--.*--&gt;/, '')
							content.gsub!(/&lt;ref.*\/ref&gt;/, '')
							content.gsub!(/\{\{.*\}\}/, '')
							content.strip!
							if (content[0, 3] == '<p>' && content[-4, 4] == '</p>')
								content.sub!('<p>', '')
								content.slice!(-4, 4)
							end
							content.strip!
							repeat = true
							while repeat
								repeat = false
								dashes.each do |dash|
									if (content.index(dash) == 0)
										content.sub!(dash, '')
										content.strip!
										repeat = true
										break
									end
								end
							end
							dateScope = 3
							dateScope = 1 if headerPath.include?('unknown') || headerPath.include?('undated')
							marker = '-'
							marker = 'f' if headerPath.include?('fiction')
 							puts "#{jd} #{dateScope} #{marker} #{type[0, 1]} #{content} <!-- #{headerPath} #{Date.jd(jd).to_s} -->"
# 							puts content if content[0, 1] == '&'
# 	 						puts "#{date} #{jd}"
#						qs = "INSERT INTO `events` (`type`, `datetime`, `content`) VALUES ('#{type}', '#{date} 00:00:00', '#{content.gsub(/[']/, '\\\\\'')}');"
# 						puts qs
# 						$conn.query(qs)
# 						puts "#{type},\"#{date}\",\"#{content}\""
						end
					end
				end
			end
		end
	end
	interestingTypes = Set.new(['birth', 'death', 'event'])
	unless (interestingTypes - seenTypes).empty?
# 		puts "WARNING: Missing types in #{year}: #{(interestingTypes - seenTypes).to_a.join(', ')}."
	end
end

path = ARGV.first
unless path
    puts "Error: No path specified."
    exit(1)
end

File::open(path, 'r') do |file|
	YAML::each_document(file) do |yearHash|
		year = yearHash.keys.first
		text = yearHash.values.first
#        puts year
 			filterEvents(year, text)
	end
end


#puts $gk_HeaderSet.to_a.sort.join("\n")
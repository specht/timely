require 'date'

#puts sprintf('%b', Date.today.jd())

today = 22
today = Date.today.jd()
first = today - 365 * 10


# step returns the next fast-forward state for a given jd
def step(jd)
    offset = 0
    
    # find first set bit
    while (((jd >> offset) & 1) == 0)
        offset += 1
    end
    
    # unset all consecutive set bits
    while (((jd >> offset) & 1) == 1)
        jd &= ~(1 << offset)
        offset += 1
    end
    
    # set next bit
    jd |= (1 << offset)
    
    return jd
end

# level defines the level, odd numbers are always level 0
def level(jd)
    return -1 if jd <= 0

    l = 0
    k = jd
    while ((k & 1) == 0)
        l += 1
        k >>= 1
    end
    return l
end


# maxlevel determines how many levels there can be at most for a given max jd
# this is just the offset of the highest bit
def maxlevel(jd)
    l = -1
    # find first set bit
    while (jd > 0)
        l += 1
        jd >>= 1
    end
    return l
end


puts "Today: #{today} (max level #{maxlevel(today)})."
rememberJds = Array.new

(first..today).each do |jd|
    nextJd = step(jd)
    #puts "#{jd}: #{nextJd} #{(nextJd > today) ? 'too big!' : ''} (level #{level(jd)})"
    rememberJds << jd if nextJd > today
end

puts "Remember: #{rememberJds.collect { |x| Date::jd(x).to_s }.join(', ')}."

=begin
puts "Slow down diffs for the following states:"

currentLevel = level(today)
currentJd = today
ml = maxlevel(today)
while (currentLevel < ml)
    currentJd -= 1
    l = level(currentJd)
    if (l > currentLevel)
        puts "Remember: #{currentJd}"
        currentLevel = l
    end
end
=end

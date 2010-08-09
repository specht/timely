def organicOrdering(first, last)
    lastCopy = last
    maxLevel = -1
    while lastCopy > 0
        maxLevel += 1
        lastCopy >>= 1
    end
    result = []
    maxLevel.downto(0) do |n|
        tempStart = 1 << n
        step = tempStart << 1
        mask = ~((1 << (n + 1)) - 1)
        start = (first & mask) | tempStart
        while start <= last
            result << start
            start += step
        end
    end
    return result
end

organicOrdering(1, 256).each do |i|
    puts i
end

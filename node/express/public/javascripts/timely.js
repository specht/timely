var DEFAULT_TIME_PER_PIXEL = 0.05;

var now = 0.0;
var time_per_pixel = DEFAULT_TIME_PER_PIXEL;
var calendar = $.calendars.instance();
var date = $.calendars.newDate();
var box_hash = {};

var search_results = Array();

var tagCount = 0;

function addTag(label, count)
{
    if (count)
    {
        if (count > 999)
        {
            if (count > 10000)
                count = '10,000+';
            else
                count = '' + Math.floor(count / 1000) + ',' + (count % 1000);
        }
        count = '(' + count + ')';
    }
    else
        count = '';
    $('#tags').append("<div id='tag-" + tagCount + "' class='tag-" + tagCount + "'><b>" + label + "</b> <span style='font-size: 11px;'>" + count + "</span></div>");
    tag = $('#tag-' + tagCount);
    tag.data('tagIndex', tagCount);
    tag.data('enabled', 1);
    tag.click(function() {
        if ($(this).data('enabled') == 1)
        {
            $(this).css('opacity', '0.5');
            $(this).data('enabled', 0);
            update();
        }
        else
        {
            $(this).css('opacity', '1.0');
            $(this).data('enabled', 1);
            update();
        }
    });
    tagCount += 1;
}

function init()
{
    now = $.calendars.newDate().toJD() - 0.3 * $('#canvas').height() * time_per_pixel;
    addTag("all events");
}


function showResults(data)
{
    canvas_width = $('#canvas').width();
    canvas_height = $('#canvas').height();
    time_range = time_per_pixel * canvas_height;
    time_min = now - time_range * 0.5;
    time_max = now + time_range * 0.5;
    time_padding = time_per_pixel * 100.0;
    
    remove_boxes = jQuery.extend({}, box_hash);
    
    function handleBox(element, tagId)
    {
        var box;
        if (!(element.id in box_hash))
        {
            useRelevance = element.relevance + (tagId > 0 ? 1000000 : 0);
            text = element.content;
            date = calendar.formatDate("M d YYYY", calendar.fromJD(element.t));
            text = date + ' &ndash; ' + text;
            if (element.type == 'd')
                text = "<span style='color: #cc0000; font-weight: bold;'>&dagger;</span>&nbsp;" + text;
            else if (element.type == 'b')
                text = "<span style='color: #4e9a06; font-weight: bold;'>&lowast;</span>&nbsp;" + text;
            $('div#canvas').append("<div class='box' style='z-index: " + useRelevance + ";' id='e" + element.id + "'>" + text + "</div>");
            box = $('#e' + element.id);
            box_hash[element.id] = box;
            box.data('relevance', useRelevance);
            box.mouseenter(function() {
                $(this).css('z-index', '999999999');
            });
            box.mouseleave(function() {
                $(this).css('z-index', $(this).data('relevance'));
            });
            if (element.fiction == 1)
                box.css('border-style', 'dashed');
    //                     box.css('left', '' + Math.random() * (canvas_width - 300) + 'px');
            box.css('left', '' + (element.relevance % 1000) / 1000.0 * (canvas_width - 300) + 'px');
            box.css('opacity', 0.0);
            box.css('top', '' + canvas_height - Math.floor((element.t - time_min) / (time_max - time_min) * canvas_height + box.height() / 2) + 'px');
            box.addClass('tag-' + tagId);
            box.animate({'opacity': 1.0});
        }
        else
        {
            box = box_hash[element.id];
            box.animate({'top': '' + canvas_height - Math.floor((element.t - time_min) / (time_max - time_min) * canvas_height + box.height() / 2) + 'px'},
                        {queue: false, duration: 200});
            delete remove_boxes[element.id];
        }
    //                 box.css('top', '' + Math.floor((element.t - time_min) / (time_max - time_min) * canvas_height - box.height() / 2) + 'px');
    }
    
    if ($('div#tag-0').data('enabled') == 1)
    {
        $.each(data, function(index, element)
        {
            handleBox(element, 0);
        });
    }
    
    $.each(search_results, function(index0, result_set)
    {
        if ($('div#tag-' + (index0 + 1)).data('enabled') == 1)
        {
            $.each(result_set, function(index, element)
            {
                handleBox(element, index0 + 1);
            });
        }
    });
    
    jQuery.each(remove_boxes, function(index, element) {
        element.remove();
        delete box_hash[index];
    });
}

function update()
{
    canvas_width = $('#canvas').width();
    canvas_height = $('#canvas').height();
    time_range = time_per_pixel * canvas_height;
    time_min = now - time_range * 0.5;
    time_max = now + time_range * 0.5;
    time_padding = time_per_pixel * 100.0;
    $.ajax({
        url: '/events/' + (time_min - time_padding) + '/' + (time_max + time_padding) + '/0',
        success: function(data)
        {
            showResults(data);
        }
    });
}

function search(query)
{
    $.ajax({
        url: '/search/' + escape(query),
        success: function(data)
        {
            if (data.results.len == 0)
            {
            }
            else
            {
                search_results.push(data.results);
                addTag(query, data.count[0].totalCount);
                if (data.results.len == 1)
                {
                    now = data.results[0].t;
                    time_per_pixel = DEFAULT_TIME_PER_PIXEL;
                }
                else
                {
                    now = (data.first[0].t + data.last[0].t) * 0.5;
                    time_per_pixel = ((data.last[0].t - data.first[0].t)) / ($('#canvas').height() - 100);
                }
                update();
            }
        }
    });
}


$(document).ready(function() {
    init();
    update();
    
    $("#searchbox").keypress(function(e)
    {
        if (e.which == 13)
        {
            search($('#searchbox').val());
            $('#searchbox').val('');
        }
    });
});

$(window).resize(function() {
    update();
});

$(document).mousewheel(function(event, delta) {
    if (event.altKey)
    {
        if (delta < 0)
            time_per_pixel *= 1.2;
        else
            time_per_pixel *= 1.0 / 1.2;
    }
    else
        now += delta * time_per_pixel * 100;
//     console.log("now: " + now + ", time_per_pixel: " + time_per_pixel);
    update();
});

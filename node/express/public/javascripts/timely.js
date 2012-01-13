now = 0.0;
time_per_pixel = 0.05;// * 100.0;
var calendar = $.calendars.instance();
var date = $.calendars.newDate();
box_hash = {};

function init()
{
    now = $.calendars.newDate().toJD() - 0.3 * $('#canvas').height() * time_per_pixel;
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
            remove_boxes = jQuery.extend({}, box_hash);
            $.each(data, function(index, element)
            {
                var box;
                if (!(element.id in box_hash))
                {
                    text = element.content;
                    date = calendar.formatDate("M d YYYY", calendar.fromJD(element.t));
                    text = date + ' &ndash; ' + text;
                    if (element.type == 'd')
                        text = "<span style='color: #cc0000; font-weight: bold;'>&dagger;</span>&nbsp;" + text;
                    else if (element.type == 'b')
                        text = "<span style='color: #4e9a06; font-weight: bold;'>&lowast;</span>&nbsp;" + text;
                    $('div#canvas').append("<div class='box' style='z-index: " + element.relevance + ";' id='e" + element.id + "'>" + text + "</div>");
                    box = $('#e' + element.id);
                    box_hash[element.id] = box;
                    box.data('relevance', element.relevance);
                    box.mouseenter(function() {
                        $(this).css('z-index', '999999999');
                    });
                    box.mouseleave(function() {
                        $(this).css('z-index', $(this).data('relevance'));
                    });
                    box.css('left', '' + Math.random() * (canvas_width - 300) + 'px');
                    box.css('opacity', 0.0);
                    box.css('top', '' + canvas_height - Math.floor((element.t - time_min) / (time_max - time_min) * canvas_height + box.height() / 2) + 'px');
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
            });
            jQuery.each(remove_boxes, function(index, element) {
                element.remove();
                delete box_hash[index];
            });
//             console.log("Got " + Object.keys(box_hash).length + " boxes on the canvas.");
        }
    })
}

$(document).ready(function() {
    init();
    update();
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
    
/*
$("input.search").keypress(function(e)
{
    if (e.which == 13)
    {
        $("li").remove();
        console.log('/search/' + $("input.search").val());
        $.ajax({
            url: '/search/' + $("input.search").val(),
            success: function(data)
            {
                mint = data[0].t;
                maxt = data[data.length - 1].t;
                $.each(data, function(index, element)
                {
                    $('div#canvas').append("<div class='box' id='e" + element.id + "'>" + element.content + "</div>");
                    $('#e' + element.id).css('top', '' + Math.floor((element.t - mint) / (maxt - mint) * 500) + 'px');
                });
            }
        });
    }
});
*/

now = 0.0;
time_per_pixel = 0.09;
var calendar = $.calendars.instance();
var date = $.calendars.newDate();
console.log(calendar);
box_hash = {};

function init()
{
    now = $.calendars.newDate().toJD();
}

function update()
{
    canvas_height = $('#canvas').height();
    time_range = time_per_pixel * canvas_height;
    time_min = now - time_range * 0.5;
    time_max = now + time_range * 0.5;
    time_padding = time_per_pixel * 100.0;
    
    $.ajax({
        url: '/events/' + (time_min - time_padding) + '/' + (time_max + time_padding) + '/0',
        success: function(data)
        {
            $('div.box').remove();
            
            $.each(data, function(index, element)
            {
                text = element.content;
                date = calendar.formatDate("M d YYYY", calendar.fromJD(element.t));
                text = date + ' &ndash; ' + text;
                if (element.type == 'd')
                    text = '&dagger;&nbsp' + text;
                else if (element.type == 'b')
                    text = '&lowast;&nbsp' + text;
                $('div#canvas').append("<div class='box' id='e" + element.id + "'>" + text + "</div>");
                box = $('#e' + element.id);
                box.css('top', '' + Math.floor((element.t - time_min) / (time_max - time_min) * canvas_height - box.height() / 2) + 'px');
            });
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
            time_per_pixel *= 1.08;
        else
            time_per_pixel *= 0.92;
    }
    else
        now -= delta * time_per_pixel * 16;
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

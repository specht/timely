var express = require('express')
  , routes = require('./routes')

var app = module.exports = express.createServer();

// Configuration

app.configure(function()
{
    app.set('views', __dirname + '/views');
    app.set('view engine', 'jade');
    app.use(express.bodyParser());
    app.use(express.methodOverride());
    app.use(express.cookieParser());
    app.use(express.session({ secret: 'your secret here' }));
    app.use(app.router);
    app.use(express.static(__dirname + '/public'));
});

app.configure('development', function()
{
    app.use(express.errorHandler({ dumpExceptions: true, showStack: true })); 
});

app.configure('production', function()
{
    app.use(express.errorHandler()); 
});

// Routes

app.get('/', routes.index);

app.get('/events/:center/:width', function(req, res)
{
    res.send('events, centered at ' + req.params.center + ', width of ' + req.params.width);
});

app.get('/events', function(req, res)
{
    res.send('got millions of events!');
});

app.listen(19810);
console.log("Express server listening on port %d in %s mode", app.address().port, app.settings.env);

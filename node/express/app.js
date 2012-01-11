var express = require('express')
  , routes = require('./routes')
  , mysql = require('mysql')
  , util = require('util');

var app = module.exports = express.createServer();

var db_client = mysql.createClient({
  user: 'se_user',
  password: 'se',
});
db_client.query('USE timely;');

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
    center = 2455913.0;
    width = 100.0;
    q = "SELECT * FROM `events` WHERE `t` > " + (center - width * 0.5) + " AND `t` < " + (center + width * 0.5) + " ORDER BY `t` ASC LIMIT 0, 100;";
    db_client.query(q, function success(err, results, fields) 
    {
        if (err)
            throw err;
        res.json(results);
    });
});

app.listen(19810);
console.log("Express server listening on port %d in %s mode", app.address().port, app.settings.env);

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

app.get('/search/:q', function(req, res)
{
    result = {};
    q = "SELECT * FROM `events` WHERE `content` LIKE '%" + req.params.q + "%' ORDER BY `relevance` DESC LIMIT 0, 30;";
    db_client.query(q, function success(err, results, fields) 
    {
        if (err)
            throw err;
        result.results = results;
        q = "SELECT * FROM `events` WHERE `content` LIKE '%" + req.params.q + "%' ORDER BY `t` ASC LIMIT 0, 1;";
        db_client.query(q, function success(err, results, fields) 
        {
            if (err)
                throw err;
            result.first = results;
            q = "SELECT * FROM `events` WHERE `content` LIKE '%" + req.params.q + "%' ORDER BY `t` DESC LIMIT 0, 1;";
            db_client.query(q, function success(err, results, fields) 
            {
                if (err)
                    throw err;
                result.last = results;
                q = "SELECT COUNT(`id`) as `totalCount` FROM `events` WHERE `content` LIKE '%" + req.params.q + "%';";
                db_client.query(q, function success(err, results, fields) 
                {
                    if (err)
                        throw err;
                    result.count = results;
                    res.json(result);
                });
            });
        });
    });
});

app.get('/events/:from/:to/:offset', function(req, res)
{
    q = "SELECT * FROM `events` WHERE `t` >= " + req.params.from + 
        " AND `t` <= " + req.params.to + 
        " ORDER BY `relevance` DESC LIMIT " + req.params.offset + ", 100;";
    db_client.query(q, function success(err, results, fields) 
    {
        if (err)
            throw err;
        res.json(results);
    });
});

app.listen(19810);
console.log("Express server listening on port %d in %s mode", app.address().port, app.settings.env);

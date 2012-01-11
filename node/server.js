var http = require('http');
var querystring = require('querystring');
var util = require('util');
var mysql = require('mysql');
var url = require('url');
var fs = require('fs');

var client = mysql.createClient({
  user: 'se_user',
  password: 'se',
});
client.query('USE timely;');

http.createServer(
    function (request, response) 
    {
        data = url.parse(request.url, true);
        path = data['pathname'];
        params = data['query'];
        if (path == '/hist')
        {
            response.writeHead(200, {'Content-Type': 'text/plain'});
            q = "SELECT * FROM `events` WHERE `t` >= '" + 
                util.format('%d', params['c'] - params['w'] * 0.5) + 
                "' AND `t` <= '" +
                util.format('%d', params['c'] + params['w'] * 0.5) + 
                "' ORDER BY `t` LIMIT 0, 100;";
            client.query(q, function success(err, results, fields) {
                             if (err)
                                 throw err;
//                              console.log(results);
                            response.end(util.inspect(results));
                         }
                   );
        }
        else
        {
//             response.writeHead(200, {'Content-Type': 'text/html'});
            if (fs.statSync("../frontend" + path).isFile())
            {
                response.end(fs.readFileSync("../frontend" + path));
            }
            else
            {
                response.writeHead(200, {'Content-Type': 'text/plain'});
                response.end("Wha?");
            }
        }
    }
).listen(19810);

console.log('Server running at http://127.0.0.1:19810/');

exports.index = function(req, res)
{
    console.log(require('util').inspect(req.url));
    res.render('index', { title: 'Timely' });
};

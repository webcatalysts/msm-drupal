'use strict'

var dblib = require('./databases'),
  bodyParser = require('body-parser');

module.exports = function (ctx) {
    const con = ctx.db;
    const server = ctx.server;

    server.use(bodyParser.urlencoded({extended: true}));
    server.get('/databases', function (req, res) {
        dblib.listDatabases(con).then(function (result) {
            res.send(result);
        });
    });

    server.post('/database/:databaseName', function (req, res) {
        dblib.saveDatabaseSettings(con.db(req.params.databaseName), req.body)
            .then(function () { res.send('ok'); })
            .catch(function (err) { res.send('error: ' + err); })
    });

    server.get('container/:containerId', function (req, res) {
        console.log(req.params.containerId);
        containers.findOne({_id: req.params.containerId}, function (err, container) {
            console.log(container.database);
            console.log(container.collection);
            var collection = dbs[container.database].collection(container.collection);
            collection.aggregate([{"$limit": 100 }]).toArray(function (err, docs) {
                res.send(200, {
                    results: docs,
                    total: docs.length,
                    schema: container.schema
                });
            });
        });
    });
}

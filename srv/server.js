var express = require('express'),
    app     = express(),
    config = require('./config'),
    DatabaseWrapper = require('./databasewrapper').DatabaseWrapper,
    DatabaseProvider = require('./databaseprovider').DatabaseProvider,
    CollectionProvider = require('./collectionprovider').CollectionProvider;

var port     = process.env.PORT || 3000,
    ip       = process.env.IP || '0.0.0.0';
    mongoURL = config.db.uri;

var databaseWrapper = new DatabaseWrapper(mongoURL);
var databaseProvider = new DatabaseProvider(databaseWrapper, 'msm');
var collectionProvider = new CollectionProvider(databaseWrapper, databaseProvider, 'msm');
databaseProvider.setCollectionProvider(collectionProvider);

app.configure(function() {
    app.set('port', port);
    app.use(express.bodyParser());
});

app.get('/databases', function (req, res) {
    databaseProvider.findAll(function(error, dbs) {
        res.send(200, dbs);
    });
});

app.get('/database/:name/enable', function(req, res) {
    databaseProvider.enable(req.params.name, function(err, result) {
        res.send(result);
    });
});

app.get('/database/:name/disable', function(req, res) {
    databaseProvider.disable(req.params.name, function(err, result) {
        res.send(result);
    });
});

app.get('/database/:name/delete', function(req,res) {
    databaseProvider.delete(req.params.name, function(err, result) {
        res.send(result);
    });
});

app.get('/database/:name/update', function(req, res) {
    databaseProvider.save(req.params.name, function(err, result) {
        res.send(result);
    });
});

app.get('/database/:db/analyze/:col', function(req, res) {
    var SchemaAnalyzer = require('./schemaanalyzer');
    var schemaAnalysis = new SchemaAnalyzer.SchemaAnalysis(req.params.db, req.params.col, databaseWrapper, collectionProvider);
    SchemaAnalyzer.analyzeSchema(schemaAnalysis, {}, function(err, result) {
        if (err) res.send(err);
        else res.send(result);
    });
});

app.post('/database/:db/analyze/:col', function(req, res) {
    var SchemaAnalyzer = require('./schemaanalyzer');
    var schemaAnalysis = new SchemaAnalyzer.SchemaAnalysis(req.params.db, req.params.col, databaseWrapper, collectionProvider);
    var options = { limit: 1*req.body.limit };
    SchemaAnalyzer.analyzeSchema(schemaAnalysis, options, function(err, result) {
        if (err) res.send(err);
        else res.send(result);
    });
});

app.get('/database/:db/count/:col', function (req, res) {
    databaseWrapper.connect(function (err, con) {
        con.db(req.params.db).collection(req.params.col).count({}, {}, function (err, count) {
            if (err) res.send({ok: 0, error: err});
            else res.send({ok: 1, count: count});
        });
    });
});

app.get('/database/:name', function(req, res) {
    databaseProvider.findById(req.params.name, function(err, dbDoc) {
        res.send(dbDoc);
    });
});

app.get('/collections', function(req, res) {
    collectionProvider.findAll(function(err, results) {
        res.send(results);
    });
});

app.get('/collections/:db', function(req, res) {
    collectionProvider.findByDatabase(req.params.db, function(err, results) {
        res.send(results);
    });
});

app.post('/collection/create/:id', function (req, res) {
    req.body.enabled = false;
    collectionProvider.save(req.params.id, req.body, function (err, result) {
        if (err) res.send({ok:0, error: err});
        else res.send(result);
    });
});

app.get('/collection/:id', function (req, res) {
    collectionProvider.findById(req.params.id, function (err, col) {
        if (err) res.send({ ok: 0, error: err});
        else res.send(col);
    });
});

app.get('/collection/:id/process', function (req, res) {
    res.send({ok: 0});
});

app.post('/collection/:id/update', function (req, res) {
    collectionProvider.save(req.params.id, req.body, function (err, result) {
        if (err) res.send({ok: 0, error: err});
        else res.send(result);
    });
});

app.get('/collection/:id/delete', function (req, res) {
    collectionProvider.delete(req.params.id, function (err, result) {
        if (err) res.send({ok: 0, error: err});
        else res.send(result);
    });
});

app.get('collection/:id/reset', function (req, res) {
    collectionProvider.resetSchema(req.params.id, function (err, result) {
        if (err) res.send(err);
        else res.send(result);
    });
});

app.get('/endpoint/:id', function (req, res) {
    res.send({ok: 0});
});

app.get('/process/:id', function (req, res) {
});

app.get('/status', function(req, res) {
    databaseWrapper.connect(function(err, con) {
        if (err) res.send({ ok: 0 });
        else {
            con.db('admin').admin().serverStatus(function(err, info) {
                con.close();
                res.send(info);
            });
        }
    });
});

app.listen(port, ip);
console.log('Server running on http://%s:%s', ip, port);
module.exports = app;

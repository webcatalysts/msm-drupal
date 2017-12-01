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

app.get('/collection/:id/process', function (req, res) {
    collectionProvider.findById(req.params.id, function (err, doc) {
        if (err || !doc) res.send({ok: 0, error: err});
        else {
            var CollectionProcess = require('./collectionprocess');
            var instance = new CollectionProcess.createInstance(doc, databaseWrapper, databaseProvider, collectionProvider);
            CollectionProcess.processInstance(instance, function (err, result) {
                if (err) res.send({ok: 0, error: err});
                else {
                    res.send(result);
                }
            });
        }
    });
});

app.post('/collection/:id/query', function (req, res) {
    collectionProvider.findById(req.params.id, function (err, doc) {
        if (err || !doc) res.send({ok: 0, error: err});
        else {
            if (!doc.enabled) {
                res.send({ok: 0, error: 'Collection not enabled.'});
            }
            databaseWrapper.connect(function (err, con) {
                var params = {
                    query: req.body.query || {},
                    project: req.body.project || false,
                    skip: req.body.skip || false,
                    limit: req.body.limit || false,
                    batchSize: req.body.batchSize || false,
                    sort: req.body.sort || false,
                }

                var db = con.db(doc.database);
                var collection = db.collection(doc.collection);

                var result = {
                    schema: doc.schema
                }

                if (doc.preExecute) {
                    eval('var preExecute = function (document, collection, params, result) { ' + doc.preExecute + ' }');
                    preExecute(doc, collection, params, result);
                }

                var cursor = con.db(doc.database).collection(doc.collection).find(params.query);

                if (params.project) {
                    cursor.project(params.project);
                }
                if (params.skip) {
                    cursor.skip(params.skip);
                }
                if (params.limit) {
                    cursor.limit(params.limit);
                }
                if (params.batchSize) {
                    cursor.batchSize(params.batchSize);
                }
                if (params.sort) {
                    cursor.sort(params.sort);
                }
                cursor.toArray(function (err, docs) {
                    if (doc.postExecute) {
                        eval('var postExecute = function (document, collection, params, result) { ' + doc.postExecute + ' }');
                        postExecute(doc, collection, params, result);
                    }
                    con.close();
                    result.results = docs;
                    result.ok = 1;
                    res.send(result);
                });
            });
        }
    });
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

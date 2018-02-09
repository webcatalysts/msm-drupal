//'use strict';

var config = require('./config');
var express = require('express');
var app = express();
var server  = require('http').Server(app);
var io = require('socket.io')(server);

SettingsProvider = require('./settingsprovider').SettingsProvider;
TestProvider = require('./testprovider').TestProvider;
DatabaseWrapper = require('./databasewrapper').DatabaseWrapper;
DatabaseProvider = require('./databaseprovider').DatabaseProvider;
CollectionProvider = require('./collectionprovider').CollectionProvider;
CollectionProcess = require('./collectionprocess').CollectionProcess;



config = Object.assign({
    applicationDatabase: 'msm',
    settingsCollectionName: 'msm_settings',
    databasesCollectionName: 'msm_databases',
    collectionsCollectionName: 'msm_collections',
    ipWhitelist: ['127.0.0.1'],
    authenticate: function (req, scope, config) {
        var ip = req.headers['x-forwarded-for'] || req.connection.remoteAddress;
        var result = config.ipWhitelist.indexOf(ip) >= 0;
        console.log('Authentication attempt from ip %s: %s', ip, result ? ' Success.' : 'Denied.');
        return result;
    },
    accessDeniedMessage: 'Access denied.',
}, config);

var port     = config.port || process.env.PORT || 3000,
    ip       = config.ip || process.env.IP || '0.0.0.0';
    mongoURL = config.db.uri || 'mongodb://localhost:27017/test';

var databaseWrapper = new DatabaseWrapper(mongoURL);
var settingsProvider = new SettingsProvider(databaseWrapper, config.settingsCollectionName);
var databaseProvider = new DatabaseProvider(databaseWrapper, 'msm');
var collectionProvider = new CollectionProvider(databaseWrapper, databaseProvider, 'msm');
var testProvider = new TestProvider(databaseWrapper, 'msm');
databaseProvider.setCollectionProvider(collectionProvider);


app.use(function (req, res, next) {
  if (!config.authenticate(req, {}, config)) {
    res.json(403, { ok: 0, error: config.accessDeniedMessage });
    return;
  }
  next();
});

var settings = {};

var bootUp = async function () {
    settings = await settingsProvider.load();
    if (settings.evalonboot) {
        eval(settings.evalonboot);
    }
}
bootUp();
app.configure(function() {
    app.set('port', port);
    app.use(express.bodyParser());
});

process.on('uncaughtException', function (err) {
    console.log(err);
});
process.on('unhandledRejection', function (err) {
    console.log(err);
});

app.get('/', function (req, res) {
  res.sendfile(__dirname + '/console.html');
});

// Get the db server status
app.get('/status', async function (req, res) {
    let con = await databaseWrapper.connect();
    let info = await con.db('admin').admin().serverStatus();
    con.close();
    res.json(info);
});

// Retrieve a list of existing and enabled databases.
app.get('/databases', async function (req, res) {
    let dbs = await databaseProvider.find();
    res.send(dbs);
});

// Retrieve details for a specific database.
app.get('/database/:name', async function (req, res) {
    let doc = await databaseProvider.findOne({_id: req.params.name});
    res.send(doc);
});

// Enable a specific database
app.get('/database/:name/enable', async function (req, res) {
    let result = await databaseProvider.enable(req.params.name);
    res.send(result);
});

// Disable a specific database
app.get('/database/:name/disable', async function (req, res) {
    let result = await databaseProvider.disable(req.params.name);
    res.send(result);
});

// Delete a specific database
app.get('/database/:name/delete', async function (req, res) {
    let result = databaseProvider.delete(req.params.name);
    res.send(result);
});

// Update a specific database
app.post('/database/:name/update', async function (req, res) {
    let result = databaseProvider.save(req.params.name, req.body);
    res.send(result);
});

// Analyze a pre-existing collection with the default limit.
app.get('/database/:db/analyze/:col', async function (req, res) {
    databaseProvider.analyzeSchema(req.params.db, req.params.col)
        .then(function (result) {
            console.log('Finished analyzing collection: %s.%s', req.params.db, req.params.col);
        })
        .catch(function (err) {
            console.log(err);
        });
    res.send({ok: 1});
});

// Analyze a pre-existing collection the a custom limit.
app.post('/database/:db/analyze/:col', async function (req, res) {
    var options = {
        limit: req.body.limit || false,
    }
    databaseProvider.analyzeSchema(req.params.db, req.params.col)
        .then(function (result) {
            console.log('Finished analyzing collection: %s.%s', req.params.db, req.params.col);
        })
        .catch(function (err) {
            console.log(err);
        });
    res.send({ok: 1});
});

// Get a collections document count
app.get('/database/:db/count/:col', async function (req, res) {
    let con = await databaseWrapper.connect();
    let count = await con.db(req.params.db).collection(req.params.col).count();
    res.send({ok: 1, count: count});
});

app.get('/collections', async function (req, res) {
    let docs = await collectionProvider.find();
    res.send(docs);
});

app.post('/collections', async function (req, res) {
    var query = req.body.query || {};
    var project = req.body.project || null;
    var limit = req.body.limit || null;
    var skip = req.body.skip || null;
    var sort = req.body.sort || null;
    let result = await collectionProvider.find(query, project, sort, limit, skip);
    res.send(result);
});

app.get('/collections/:db', async function (req, res) {
    let docs = await collectionProvider.find({database: req.params.db});
    res.send(docs);
});

app.post('/collection/create/:id', async function (req, res) {
    req.body.enabled = false;
    console.log(req.body);
    let result = await collectionProvider.save(req.params.id, req.body);
    res.send(result);
});

app.get('/collection/:id', async function (req, res) {
    let doc = await collectionProvider.findOne({_id: req.params.id});
    res.send(doc);
});

app.get('/collection/:id/count', async function (req, res) {
    let count = await collectionProvider.countDocuments(req.params.id);
    res.send({ok: 1, count: count});
});

app.post('/collection/:id/count', async function (req, res) {
    var query = req.params.body.query || {};
    var options = req.params.body.options || {};
    let count = await collectionProvider.countDocuments(req.params.id, query, options);
    res.send({ok: 1, count: count});
});

app.post('/collection/:id/query', async function (req, res) {
    res.setHeader('Access-Control-Allow-Origin', 'http://msm.dd:8083');
    res.setHeader('Access-Control-Allow-Methods', 'GET');
    res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type');
    var params = {
        query: req.body.query || {},
        project: req.body.project || false,
        skip: req.body.skip || false,
        limit: req.body.limit || false,
        batchSize: req.body.batchSize || false,
        sort: req.body.sort || false
    };
    var options = {
        flattenSchema: false,
        flattenResults: false,
    }
    if (req.body.options) {
        options = Object.assign({}, options, req.body.options);
    }
    console.log(options);
    let result = await collectionProvider.query(req.params.id, params, options);
    res.send(result);
});

app.get('/collection/:id/schema', async function (req, res) {
    res.setHeader('Access-Control-Allow-Origin', 'http://msm.dd:8083');
    res.setHeader('Access-Control-Allow-Methods', 'GET');
    res.setHeader('Access-Control-Allow-Headers', 'X-Requested-With,content-type');
    var options = {
        project: req.query.project || null,
        flatten: req.query.flatten || false,
    }
    var result = await collectionProvider.getSchema(req.params.id, options);
    res.send(result);
});

app.get('/collection/:id/socket', async function (req, res) {
});

app.get('/collection/:id/dependents', async function (req, res) {
    var result = await collectionProvider.getDependents(req.params.id);
    res.send(result);
});

app.get('/collection/:id/analyze', async function (req, res) {
    collectionProvider.analyzeSchema(req.params.id, {})
        .then(function (result) {
            console.log('Finished analyzing collection: %s', req.params.id);
        })
        .catch(function (err) {
            console.log(err);
        });
    res.send({ok: 1});
});

app.post('/collection/:id/analyze', async function (req, res) {
    var options = { limit: 1*req.body.limit };
    collectionProvider.analyzeSchema(req.params.id, options)
        .then(function (result) {
            console.log('Finished analyzing collection: %s', req.params.id);
        })
        .catch(function (err) {
            console.log(err);
        });
    res.send({ok: 1});
});

app.post('/collection/:id/update', async function (req, res) {
    let result = await collectionProvider.save(req.params.id, req.body);
    res.send(result);
});

app.get('/collection/:id/delete', async function (req, res) {
    let result = await collectionProvider.deleteOne({_id: req.params.id});
    res.send(result);
});

app.get('/collection/:id/reset', async function (req, res) {
    let result = await collectionProvider.resetSchema(req.params.id);
    return result;
});

app.post('/collection/:id/process', async function (req, res) {
    try {
        let updateResult = await collectionProvider.save(req.params.id, req.body);
        console.log('Updating and processing collection: %s', req.params.id);
        let collectionProcess = new CollectionProcess(
            req.params.id,
            databaseWrapper,
            databaseProvider,
            collectionProvider,
            settings
        );
        console.log('Successfully updated collection: %s. Running colleciton process..', req.params.id);
        collectionProcess.run({analyzeSchema:true});
        res.send({ok: 1});
    }
    catch (error) {
        console.log('Error processing collection: %s', error);
        res.status(500).send('Error processing collection: %s', error);
    }
});

app.get('/collection/:id/process', function (req, res) {
    try {
        console.log('Updating and processing collection: %s', req.params.id);
        let collectionProcess = new CollectionProcess(
            req.params.id,
            databaseWrapper,
            databaseProvider,
            collectionProvider
        );
        console.log('Successfully updated collection: %s. Running colleciton process..', req.params.id);
        collectionProcess.run({analyzeSchema:true});
        res.send({ok: 1});
    }
    catch (error) {
        console.log('Error processing collection: %s', error);
        res.status(500).send('Error processing collection: %s', error);
    }
});

app.get('/process', async function (req, res) {
    try {
        console.log('Processing ALL');
        let collectionProcess = new CollectionProcess(
            null,
            databaseWrapper,
            databaseProvider,
            collectionProvider
        );
        console.log('Running colleciton process..');
        collectionProcess.runAll();
    }
    catch (error) {
        console.log('Error processing collection: %s', error);
        res.status(500).send('Error processing collection: %s', error);
    }
    res.send({ok:1});
});
app.get('/process/list', async function (req, res) {
    let collectionProcess = new CollectionProcess(
        null,
        databaseWrapper,
        databaseProvider,
        collectionProvider
    );
    await collectionProcess.startRun();
    res.send(collectionProcess.items);
});

app.get('/settings', async function (req, res) {
    let result = await settingsProvider.load();
    res.send(result);
});
app.post('/settings', async function (req, res) {
    if (Object.keys(req.body)) {
        console.log(req.body);
        let result = await settingsProvider.saveMany(req.body);
        let settings = await settingsProvider.load();
        res.send(settings);
        process.exit(1);
    }
    else res.send(500, 'Empty request');
});

app.get('/test/:id', async function (req, res) {
    let testDoc = await testProvider.findOne({_id: req.params.id}, {_id: 1});
    if (testDoc) {
        res.json(testDoc);
    }
    else res.json(404, {ok: 0, error: "Test not found"});
});

app.post('/test/:id', async function (req, res) {
    let result = await testProvider.save(req.params.id, req.body);
    res.json(result);
});
app.get('/test/:id/run', async function (req, res) {
    let testDoc = await testProvider.findOne({_id: req.params.id}, {_id: 1});
    if (testDoc) {
        var result = await testProvider.runTest(testDoc._id);
        res.send({ok: 1, result: result});
    }
    else res.send(404, {ok: 0, error: "Test not found"});
});

app.get('/tests', async function (req, res) {
    let result = await testProvider.find();
    res.json(result);
});

app.get('/tests/run', async function (req, res) {
    testProvider.runAllTests();
    res.json({ok: 1});
});

app.get('/restart', function (req, res) {
    res.send({ok: 1});
    process.exit(1);
});

io.on('connect', (socket) => {
    io.emit('textMessage', 'Connected from: ' + socket.client.conn.remoteAddress);
    console.log('Connected from: ' + ip);
    console.log(Object.keys(socket.client.conn.remoteAddress));
    console.log(socket.client.conn.remoteAddress);
    console.log(socket.client.id);
});

(async function (io, globalIO) {
    let con;
    let serverStatus;
    var refreshServerStatus = async function (socket) {
        con = await databaseWrapper.connect();
        serverStatus = await con.db('admin').admin().serverStatus();
        globalIO.emit('textMessage', 'status requested');
        socket.emit('data', serverStatus);
    };
    io.on('connect', (socket) => {
        io.emit('textMessage', 'Connected to /status from: ' + socket.client.conn.remoteAddress);
        refreshServerStatus(socket);
        socket.on('refresh', function () {
            console.log('refresh requested');
            refreshServerStatus(socket);
        });
    });
})(io.of('/status'), io);

(async function (io, globalIO) {
    io.on('log', function (...args) {
        console.log(args);
        io.emit('textMessage', 'xyz');
    });
})(io.of('/console'), io);

server.listen(port, ip);
console.log('Server running on http://%s:%s', ip, port);
module.exports = app;

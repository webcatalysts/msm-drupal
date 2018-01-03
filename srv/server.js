var express = require('express'),
    app     = express(),
    config = require('./config'),
    SettingsProvider = require('./settingsprovider').SettingsProvider,
    DatabaseWrapper = require('./databasewrapper').DatabaseWrapper,
    DatabaseProvider = require('./databaseprovider').DatabaseProvider,
    CollectionProvider = require('./collectionprovider').CollectionProvider;


config = Object.assign({
    applicationDatabase: 'msm',
    settingsCollectionName: 'msm_settings',
    databasesCollectionName: 'msm_databases',
    collectionsCollectionName: 'msm_collections',
}, config);

var port     = process.env.PORT || 3000,
    ip       = process.env.IP || '0.0.0.0';
    mongoURL = config.db.uri;

var databaseWrapper = new DatabaseWrapper(mongoURL);
var settingsProvider = new SettingsProvider(databaseWrapper, config.settingsCollectionName);
var databaseProvider = new DatabaseProvider(databaseWrapper, 'msm');
var collectionProvider = new CollectionProvider(databaseWrapper, databaseProvider, 'msm');
databaseProvider.setCollectionProvider(collectionProvider);

var settings = {};

var bootUp = async function () {
    settings = await settingsProvider.load();
    console.log(settings);
    if (settings.evalonboot) {
        eval(settings.evalonboot);
    }
}
bootUp();

app.configure(function() {
    app.set('port', port);
    app.use(express.bodyParser());
});

// Get the db server status
app.get('/status', async function (req, res) {
    let con = await databaseWrapper.connect();
    let info = await con.db('admin').admin().serverStatus();
    con.close();
    res.send(info);
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
    var params = {
        query: req.body.query || {},
        project: req.body.project || false,
        skip: req.body.skip || false,
        limit: req.body.limit || false,
        batchSize: req.body.batchSize || false,
        sort: req.body.sort || false
    };
    let result = await collectionProvider.query(req.params.id, params);
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
    var CollectionProcess = require('./collectionprocess').CollectionProcess;
    try {
        let updateResult = await collectionProvider.save(req.params.id, req.body);
        console.log('Updating and processing collection: %s', req.params.id);
        let collectionProcess = new CollectionProcess(
            req.params.id,
            databaseWrapper,
            databaseProvider,
            collectionProvider
        );
        console.log('Successfully updated collection: %s. Running colleciton process..', req.params.id);
        collectionProcess.run();
        res.send({ok: 1});
    }
    catch (error) {
        console.log('Error processing collection: %s', error);
        res.status(500).send('Error processing collection: %s', error);
    }
});

app.get('/collection/:id/process', function (req, res) {
    let CollectionProcess = require('./collectionprocess').CollectionProcess;
    try {
        console.log('Updating and processing collection: %s', req.params.id);
        let collectionProcess = new CollectionProcess(
            req.params.id,
            databaseWrapper,
            databaseProvider,
            collectionProvider
        );
        console.log('Successfully updated collection: %s. Running colleciton process..', req.params.id);
        collectionProcess.run();
        res.send({ok: 1});
    }
    catch (error) {
        console.log('Error processing collection: %s', error);
        res.status(500).send('Error processing collection: %s', error);
    }
});

app.get('/process', async function (req, res) {
    let CollectionProcess = require('./collectionprocess').CollectionProcess;
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
    }
    else res.send(500, 'Empty request');
});

app.listen(port, ip);
console.log('Server running on http://%s:%s', ip, port);
module.exports = app;

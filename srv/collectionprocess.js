var Promise = require('promise');

var CollectionProcess = function (collectionId, databaseWrapper, databaseProvider, collectionProvider) {
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.collectionProvider = collectionProvider;
    this.collectionId = collectionId;
}

CollectionProcess.prototype.runAll = async function () {
    this.db = await this.databaseProvider.getDatabase();
    this.storageCollection = this.db.collection('msm_process');

    console.log('Starting Run All Process.');
    await this.startRun();
    console.log('Getting next item..');
    while (this.runAllNext()) {
        this.storageCollection.save({collectionId: this.collection._id});
        console.log(this.collection._id);
    }
    console.log('Run All Process Complete.');
}

CollectionProcess.prototype.startRun = async function () {
    var timeout = 1999;
    let check = true;
    while (timeout > 0) {
        check = await this.checkProcess(false);
        if (check) {
            console.log('Another process is already running.');
            wait(500);
            timeout +=-500;
        }
        else break;
    }
    if (timeout <= 0) {
        console.log('Operation timed out..');
        return;
    }
    this.items = await this.collectionProvider.find({
        enabled: true,
    }, {}, { weight: 1 });
}

CollectionProcess.prototype.checkProcess = async function () {
    return await this.storageCollection.count();
}

CollectionProcess.prototype.runAllNext = function () {
    if (this.collection = this.items.current()) {
        this.collectionId = this.collection._id;
        return (typeof this.collection === 'undefined');
    }
    return false;
}

CollectionProcess.prototype.run = async function () {
    this.collection = await this.collectionProvider.findOne({_id: this.collectionId});
    if (!this.collection.source) {
        throw new Error('Collection %s missing source attribute.', this.collection._id);
    }
    this.source = await this.collectionProvider.findOne({_id: this.collection.source});
    if (!this.collection.source) {
        throw new Error('Source collection %s not found.', this.collect.source);
    }
    var numSourceDocuments = await this.collectionProvider.countDocuments(this.collection.source);
    if (numSourceDocuments <= 0) {
        throw new Error('Source collection %s is empty.');
    }

    var result = {};
    var instance = this;
    let con = await instance.databaseWrapper.connect();
    var sourceDatabaseName = instance.source.database;
    var sourceDatabase = con.db(sourceDatabaseName);
    var sourceCollectionName = instance.source.collection;
    var sourceCollection = sourceDatabase.collection(sourceCollectionName);
    if (instance.collection.persist) {
        var destinationDatabaseName = instance.collection.database;
        var destinationDatabase = con.db(destinationDatabaseName);
        var destinationCollectionName = instance.collection.collection;
        var destinationCollection = destinationDatabase.collection(destinationCollectionName);
    }
    if (instance.collection.dependencies && instance.collection.dependencies.length) {
        let deps = await instance.collectionProvider.find({_id: { "$in": instance.collection.dependencies }});
        var numD = deps.length;
        var dependencies = {};
        for (var d = 0; d < numD; d++) {
            var dep = deps[d];
            dependencies[dep._id] = {
                databaseName: dep.database,
                collectionName: dep.collection,
                database: con.db(dep.database),
                collection: con.db(dep.database).collection(dep.collection),
            }
        }
    }
    if (instance.collection.preProcess) {
        console.log('Pre-processing: %s', instance.collection._id);
        eval('var preProcess = async function(instance, result) { ' + instance.collection.preProcess + '};');
        await preProcess(instance, result); 
    }
    var callback = async function (err, res) {
        if (err) {
            console.log('Failed to process %s: %s', instance.collection._id, err);
            con.close();
        }
        else {
            if (!instance.collection.schemaCustomized) {
                await instance.collectionProvider.analyzeSchema(instance.collectionId);
            }
            console.log('Successfully processed collection: ', instance.collection._id);
            if (instance.collection.postProcess) {
                console.log('Post-processing: %s', instance.collection._id);
                eval('var postProcess = function(instance, result) { ' + instance.collection.postProcess + '};');
                await postProcess(instance, result);
            }
            con.close();
        }
    }
    console.log('Processing collection: %s', instance.collection._id);
    switch (instance.collection.type) {
        case 'mapReduce':
            break;
    return true;
            break;
        case 'aggregation':
            var handler = CollectionProcessHandlerAggregation;
            break;
        case 'custom':
            console.log('Processing custom eval collection: ', instance.collection._id);
            eval('var evalCustom = async function () { ' + instance.collection.eval.code + ' };');
            await evalCustom();
            break;
        default:
            throw new Error('Unknown collection type or invalid collection.');
    }
}

module.exports = {
    CollectionProcess: CollectionProcess,
}

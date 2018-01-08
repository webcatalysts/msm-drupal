var Promise = require('promise');

var CollectionProcess = function (collectionId, databaseWrapper, databaseProvider, collectionProvider) {
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.collectionProvider = collectionProvider;
    this.collectionId = collectionId;
}

CollectionProcess.prototype.runAll = async function () {
    console.log('Starting Run All Process.');
    await this.startRun();
    console.log('Getting next item..');
    try {
        while (this.runAllNext()) {
            await this.storageCollection.save({collectionId: this.collectionId});
            var cp = new CollectionProcess(this.collectionId, this.databaseWrapper, this.databaseProvider, this.collectionProvider);
            await cp.run();
            this.storageCollection.remove({collectionId: this.collectionId});
        }
    }
    catch (err) {
        console.log('Error: %s', err);
        console.log(err);
        if (this.collectionId) {
            this.storageCollection.remove({collectionId: this.collectionId});
            this.collection = null;
            delete this.items;
            delete this.itemKeys;
        }
    }
    console.log('Run All Process Complete.');
}

CollectionProcess.prototype.startRun = async function () {
    this.db = await this.databaseProvider.getDatabase();
    this.storageCollection = this.db.collection('msm_process');

    var timeout = 1999;
    let check = true;
    while (timeout > 0) {
        check = await this.checkProcess(false);
        if (check) {
            console.log('Another process is already running.');
            //wait(500);
            timeout +=-500;
        }
        else break;
    }
    if (timeout <= 0) {
        console.log('Operation timed out..');
        return;
    }
    this.items = await this.collectionProvider.find({
    //}, { _id: 1, dependencies: 1, source: 1 }, { weight: 1 });
    }, { _id: 1, dependencies: 1, source: 1 });

    this.itemKeys = [];
    var numI = this.items.length;
    for (var i = 0; i < numI; i++) {
        this.itemKeys.push(this.items[i]._id);
    }
    for (var i = 0; i < numI; i++) {
        var item = this.items[i];
        if (typeof item.dependencies === 'undefined') {
            item.dependencies = [];
        }
        if (item.source) {
            //item.dependencies.unshift(item.source);
            item.dependencies.push(item.source);
        }
        item.totalDependencies = item.dependencies.length;
        for (var d = 0; d < item.totalDependencies; d++) {
            var dep = item.dependencies[d];
            if (this.itemKeys.indexOf(dep) === -1) {
                console.log('Collection %s has a missing dependency: %s', item._id, dep);
            }
        }
        this.items[i] = item;
    }
    for (var i = 0; i < numI; i++) {
        var item = this.items[i];
        item.weight = item.weight ? item.weight : 0;
        for (var i2 = 0; i2 < numI; i2++) {
            if (i === i2) continue;
            item2 = this.items[i2];
            item2.weight = item2.weight ? item2.weight : 0;
            if (item.dependencies.indexOf(item2._id) !== -1) {
                if (item.weight < item2.weight) {
                    item.weight = item2.weight;
                }
                item.weight++;
            }
            else if (item2.dependencies.indexOf(item._id) !== -1) {
                if (item.weight > item2.weight) {
                    item.weight = item2.weight;
                }
                item.weight--;
            }
            else {
                item.weight += (item2.weight - item.weight);
            }
            this.items[i2] = item2;
        }
        this.items[i] = item;
    }
    //this.items.sort(function (a, b) { return a.weight - b.weight; });
    this.items = SortProcessesByDependencies(this.items);
    for (var i = 0; i < this.items.length; i++) {
        console.log(i + ': %s (%d)', this.items[i]._id, this.items[i].dependencies.length);
    }
    this.iterator = 0;
    return true;
}

CollectionProcess.prototype.checkProcess = async function () {
    return await this.storageCollection.count();
}

CollectionProcess.prototype.runAllNext = function () {
    var index = this.iterator;
    if (this.items[index]) {
        this.iterator++;
        this.collectionId = this.items[index]._id;
        return true;
    }
    delete this.collection;
    delete this.collectionId;
    return false;
}

CollectionProcess.prototype.run = async function (options = {}) {
    console.log('Processing collection: %s', this.collectionId);
    options = Object.assign({analyzeSchema: false}, options);
    this.collection = await this.collectionProvider.findOne({_id: this.collectionId});
    if (!this.collection) {
        throw new Error('Collection %s could not be found.', this.collectionId);
    }
    if (this.collection.type && !this.collection.source) {
        throw new Error('Collection %s missing source attribute.', this.collection._id);
    }
    else if (this.collection.type && this.collection.source) {
        this.source = await this.collectionProvider.findOne({_id: this.collection.source});
        if (!this.source) {
            throw new Error('Source collection %s not found.', this.collect.source);
        }
        var numSourceDocuments = await this.collectionProvider.countDocuments(this.collection.source);
        if (numSourceDocuments <= 0) {
            throw new Error('Source collection ' + this.source._id + ' is empty.');
        }
    }

    var result = {};
    var instance = this;
    let con = await instance.databaseWrapper.connect();
    if (instance.source) {
        var sourceDatabaseName = instance.source.database;
        var sourceDatabase = con.db(sourceDatabaseName);
        var sourceCollectionName = instance.source.collection;
        var sourceCollection = sourceDatabase.collection(sourceCollectionName);
    }
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
            if (options.analyzeSchema && !instance.collection.schemaCustomized) {
                await instance.collectionProvider.analyzeSchema(instance.collectionId);
            }
            if (instance.collection.postProcess) {
                console.log('Post-processing: %s', instance.collection._id);
                eval('var postProcess = async function(instance, result) { ' + instance.collection.postProcess + '};');
                await postProcess(instance, result);
                console.log('Finished post-processing: %s', instance.collection._id);
            }
            console.log('Successfully processed collection: ', instance.collection._id);
            con.close();
        }
    }
    switch (instance.collection.type) {
        case 'mapReduce':
            break;
        case 'aggregation':
            break;
        case 'custom':
            //console.log('Processing custom eval collection: ', instance.collection._id);
            eval('var evalCustom = async function () { ' + instance.collection.eval.code + ' };');
            await evalCustom();
            break;
        default:
            await callback(null);
            //throw new Error('Unknown collection type or invalid collection.');
            break;
    }
}

var SortProcessesByDependencies = function (collections) {
    var tmpCol = {};
    var numCollections = collections.length;
    var collectionIds = Object.keys(collections);
    for (var i = 0; i < numCollections; i++) {
        var id = collections[i]._id;
        tmpCol[id] = collections[i];
    }
    collections = tmpCol;
    var sortedArr = [];
    var visited = {};
    for (var id in collections) {
        console.log(id);
        var item = collections[id];
        Visit(item, visited, sortedArr, collections);
    }
    return sortedArr;
}
var Visit = function (item, visited, sorted, collections) {
    var itemId = item._id;
    if (typeof visited[itemId] === 'undefined') {
        visited[itemId] = 1;
        var numD = item.dependencies.length;
        for (var d = 0; d < numD; d++) {
            var dep = item.dependencies[d];
            Visit(collections[dep], visited, sorted, collections);
        }
        sorted.push(item);
    }
    else {
    }
}

module.exports = {
    CollectionProcess: CollectionProcess,
}


CollectionProvider = function(databaseWrapper, databaseProvider, databaseName, collectionName) {
    this.databaseWrapper = databaseWrapper;
    this.databaseProvider = databaseProvider;
    this.databaseName = databaseName || 'msm';
    this.collectionName = collectionName || 'msm_collections';
}

CollectionProvider.prototype.getCollection = function(callback) {
    var provider = this;
    this.databaseWrapper.connect(function(err, con) {
        if (err) callback(err);
        else {
            var db = con.db(provider.databaseName);
            callback(null, db.collection(provider.collectionName), db, con);
        }
    });
}

CollectionProvider.prototype.findAll = function(callback) {
    var thisObj = this;
    this.getCollection(function(err, col, db, con) {
        col.find().toArray(function(err, docs) {
            con.close();
            if (err) callback(err);
            else callback(null, docs);
        });
    });
}

CollectionProvider.prototype.findByDatabase = function(dbName, callback) {
    var thisObj = this;
    this.getCollection(function(err, col, db, con) {
        col.find({database: dbName}).toArray(function(err, docs) {
            con.close();
            callback(null, docs);
        });
    });
}

CollectionProvider.prototype.findById = function (id, callback) {
    this.getCollection(function (err, col, db, con) {
        if (err) callback(err);
        else {
            col.findOne({_id: id}, {}, function (err, result) {
                con.close();
                callback(null, result);
            });
        }
    });
}

CollectionProvider.prototype.delete = function (id, callback) {
    this.getCollection(function (err, col, db, con) {
        if (err) callback(err);
        else {
            col.deleteOne({_id:id}, {}, function (err, result) {
                con.close();
                if (err) callback(err);
                else callback(null, result);
                console.log(id);
            });
        }
    });
}

CollectionProvider.prototype.deleteByDatabase = function (dbName, callback) {
    this.getCollection(function (err, col, db, con) {
        if (err) callback(err);
        else {
            col.deleteMany({database: dbName}, {}, function (err, result) {
                con.close();
                if (err) callback(err);
                else callback(null, result);
            });
        }
    });
}

CollectionProvider.prototype.save = function(id, data, callback) {
    this.getCollection(function(err, col, db, con) {
        col.updateOne({_id: id}, data, {upsert:true}, function (err, result) {
            con.close();
            if (err) callback(err);
            else callback(null, result);
        });
    });
}

CollectionProvider.prototype.resetSchema = function (databaseName, collectionName, callback) {
    this.save(databaseName, collectionName, { "$set": { enabled: false }, "$unset": { schema: "" } }, function(err, result) {
        if (err) callback(err);
        else callback(null, result);
    });
}

CollectionProvider.makeId = function (databaseName, collectionName) {
    return databaseName + '.' + collectionName;
}

exports.CollectionProvider = CollectionProvider;
exports.makeId = CollectionProvider.makeId;


DatabaseWrapper = function(url, options) {
    this.url = url;
    this.options = options;
    this.mongodb = require('mongodb');
    this.connection = null;
}

DatabaseWrapper.prototype.connect = function (callback) {
    this.mongodb.connect(this.url, function (err, con) {
        this.connection = con;
        callback(err, this.connection);
    });
}

DatabaseWrapper.prototype.getCollection = function (dbName, collectionName, callback) {
    this.connect(function (err, con) {
        if (err) {
            callback(err);
        }
        else {
            var db = con.db(dbName);
            var col = db.collection(collectionName);
            callback(null, col, db, con);
        }
    });
}

DatabaseWrapper.prototype.close = function() {
    this.connection.close();
}

exports.DatabaseWrapper = DatabaseWrapper;

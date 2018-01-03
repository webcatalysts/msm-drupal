
DatabaseWrapper = function(url, options) {
    this.url = url;
    this.options = options;
    this.mongodb = require('mongodb');
    this.connection = null;
}

DatabaseWrapper.prototype.connect = async function () {
    this.connection = await this.mongodb.connect(this.url);
    return this.connection;
}

DatabaseWrapper.prototype.getCollection = async function (dbName, collectionName) {
    await this.connect();
    let db = this.connection.db(dbName);
    return {
        con: this.connection,
        db: db,
        col: db.collection(collectionName),
    };
}

DatabaseWrapper.prototype.getDatabase = async function (dbName) {
    await this.connect();
    return this.connection.db(dbName);
}

DatabaseWrapper.prototype.close = function() {
    this.connection.close();
}

exports.DatabaseWrapper = DatabaseWrapper;

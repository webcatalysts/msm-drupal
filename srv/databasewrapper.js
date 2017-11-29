
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

DatabaseWrapper.prototype.close = function() {
    this.connection.close();
}

exports.DatabaseWrapper = DatabaseWrapper;

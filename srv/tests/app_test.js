'use strict';

var server   = require('../server'),
    chai     = require('chai'),
    chaiHTTP = require('chai-http'),
    should   = chai.should();

var reqServer = process.env.HTTP_TEST_SERVER || server;
chai.use(chaiHTTP);

describe('Routes tests', function () {
    it('GET to /databases should return 200', function (done) {
        chai.request(server)
        .get('/databases')
        .end(function(err, res) {
            res.should.have.status(200)
            done();
        });
    });
});

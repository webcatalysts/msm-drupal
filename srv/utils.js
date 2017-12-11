'use strict';

var _ = require('lodash');
var util = require('util');
var evalConvertor = {
    'string': JSON.stringify,
    'object': JSON.stringify,
}

var jsVarDefinition = function (value, name) {
    return util.format('this.%s=%s;', name, (evalConvertor[typeof value] || _.identity)(value));
}

var buildVarietyParams = function (collection, opts) {
    return _.map(
        _.assign({}, {'collection': collection}, opts),
        jsVarDefinition
    ).join('');
};

module.exports = {
    'buildVarietyParams': buildVarietyParams
}

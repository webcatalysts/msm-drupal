var MongoSchemaManager = {}

MongoSchemaManager.projectSchema = function (project, schema) {
  var removeFields = [];
  var includeFields = [];
  for (var pfield in project) {
    if (project[pfield]) {
      includeFields.push(pfield);
    }
    else {
      removeFields.push(pfield);
    }
  }
  if (removeFields.indexOf('_id') === -1) {
    includeFields.push('_id');
  }
  return this.projectSchemaRecurse(schema, includeFields, removeFields);
}

MongoSchemaManager.projectSchemaRecurse = function (schema, includeFields, removeFields, ns) {
  var result = {};
  for (var field_name in schema) {
    var cns = ns ? ns + '.' + field_name : field_name;
    if (removeFields.indexOf(cns) !== -1) continue;
    var field_info = schema[field_name];
    if (includeFields.indexOf(cns) !== -1) {
      result[field_name] = field_info;
    }
    else if (field_info.children) {
      var children = this.projectSchemaRecurse(field_info.children, includeFields, removeFields, cns);
      if (Object.keys(children).length) {
        field_info.children = children;
        result[field_name] = field_info;
      }
    }
  }
  return result;
}

MongoSchemaManager.flattenSchemaFields = function (schema, ns) {
  var result = {};
  for (var f in schema) {
    var v = schema[f];
    if (ns) {
      v.parent = ns;
      var cns = ns + '.' + f;
      v.parents = cns.split('.');
    }
    else {
      var cns = f;
      v.parents = [f];
    }
    if (v.type == 'group' || v.type == 'array') {
      if(v.children && Object.keys(v.children).length) {
        var ch = v.children;
        delete v.children;
        result = Object.assign({}, this.flattenSchemaFields(ch, cns), result);
      }
    }
    result[cns] = v;
  } 
  return result;
}
MongoSchemaManager.flattenResults = function (results) {
  var ret = [];
  results.forEach(function (result) {
      ret.push(this.flattenResult(result));
  }.bind(this));
  return ret;
}
MongoSchemaManager.flattenResult = function (result, ns = null) {
  var ret = {}
  if (typeof result === 'object') {
    Object.keys(result).forEach(function(key) {
      var cns = ns ? ns + '.' + key : key;
      var value = result[key];
      if (value instanceof Array) {
        ret[cns] = value;
      }
      else if (value instanceof Object) {
        var subret = this.flattenResult(value, cns);
        ret = Object.assign({}, ret, subret);
      }
      else {
        ret[cns] = value;
      }
    }.bind(this));
  }
  return ret;
}
MongoSchemaManager.expandSchema = function (fields, overrideWeights = false) {
  var fieldNames = Object.keys(fields).sort();
  var numFields = fieldNames.length;
  var schema = {};
  var counts = {};
  for (var f = 0; f < numFields; f++) {
    var fieldName = fieldNames[f];
    var field = fields[fieldName];
    var pstr = field.parents.join('.children.');
    var countsIdx = pstr.length ? pstr : '____root____';
    if (typeof counts[countsIdx] === 'undefined') {
      counts[countsIdx] = 0;
    }
    counts[countsIdx]++;
    if (overrideWeights) {
      field.display.weight = counts[countsIdx];
    }
    var parents = pstr.split('.');
    schema = this.setNestedValue(schema, parents, field);
  }
  return schema;
}
MongoSchemaManager.mergeSchema = function (a,b) {
  return _.merge(a,b);
}

MongoSchemaManager.setNestedValue = function (obj, parents, value, force = false) {
  return _.set(obj, parents.join('.'), value);
}
MongoSchemaManager.sortSchema = function (schemaIn) {
  var schemaOut = {};
  var fields = [];
  var fieldNames = Object.keys(schemaIn);
  var numFields = fieldNames.length;
  for (var i = 0; i < numFields; i++) {
    var f = fieldNames[i];
    var field = schemaIn[f];
    field.display = field.display && Object.keys(field.display).length ? field.display : {};
    field.display.weight = typeof field.display.weight === 'undefined' ? 0 : field.display.weight;
    field.__fieldName = f;
    if (field.children && Object.keys(field.children).length) {
      field.children = this.sortSchema(schemaIn[f].children);
    }
    fields.push(field);
  }
  fields.sort(function(a,b) {
    if (a.display.weight < b.display.weight) return -1;
    if (a.display.weight > b.display.weight) return 1;
    return 0;
  });
  for (var i = 0; i < numFields; i++) {
    var f = fields[i].__fieldName;
    delete fields[i].__fieldName;
    schemaOut[f] = fields[i];
  }
  return schemaOut;
}

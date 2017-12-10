

function projectSchema(project, schema) {
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
  return projectSchemaRecurse(schema, includeFields, removeFields);
}

function projectSchemaRecurse(schema, includeFields, removeFields, ns) {
  var result = {};
  for (var field_name in schema) {
    var cns = ns ? ns + '.' + field_name : field_name;
    if (removeFields.indexOf(cns) !== -1) continue;
    var field_info = schema[field_name];

    if (includeFields.indexOf(cns) !== -1) {
      result[field_name] = field_info;
    }
    else if (field_info.children) {
      var children = projectSchemaRecurse(field_info.children, includeFields, removeFields, cns);
      if (Object.keys(children).length) {
        field_info.children = children;
        result[field_name] = field_info;
      }
    }
  }
  return result;
}

function fieldIsIncludedInProject(project, field_name) {
  if (typeof project[field_name] !== 'undefined') {
    return project[field_name];
  }
  var removeFields = [];
  var includeFields = [];
  for (var pfield in project) {
    if (project[pfield]) {
      includeFields.push(pfield);
    }
    else removeFields.push(pfield);
  }

  var numR = removeFields.length;
  var numI = includeFields.length;
  for (var r = 0; r < numR; r++) {
  }
  for (var i = 0; i < numI; i++) {
  }
  return true;
}

function flattenSchemaFields(schema, ns) {
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
      if(v.children && v.children.length) {
        result = Object.assign(result, flattenSchemaFields(v.children, cns));
        delete v.children;
      }
    }
    result[cns] = v;
  }
  return result;
}

function expandSchema(fields) {
}

module.exports = {
    flattenSchemaFields: flattenSchemaFields,
    projectSchema: projectSchema,
}

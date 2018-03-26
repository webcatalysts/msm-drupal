<?php

function msm_collection_list($client_server_id = 'default:default') {
  return drupal_get_form('msm_collection_list_form', $client_server_id);
}

function msm_collection_list_form($form, &$form_state, $client_server_id) {
  form_load_include($form_state, 'inc', 'msm', 'includes/collection');
  $values = empty($form_state['values']['filters']) ? array() : $form_state['values']['filters'];
  $values += array(
    'client_server_id' => $client_server_id,
    'database' => null,
  );
  list($client_id, $server_id) = explode(':', $values['client_server_id']);
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#attributes' => array('class' => array('container-inline')),
    '#tree' => TRUE,
  );
  $ajax = array(
    'callback' => 'msm_collection_list_form_ajax',
    'wrapper' => $html_id,
  );
  $form['filters']['client_server_id'] = array(
    '#title' => t('Server'),
    '#type' => 'select',
    '#options' => msm_clients_server_options(),
    '#default_value' => $values['client_server_id'],
    '#ajax' => $ajax,
  );
  $form['filters']['database'] = array(
    '#title' => t('Database'),
    '#type' => 'select',
    '#options' => msm_database_options($server_id, $client_id),
    '#default_value' => $values['database'],
  );
  $form['filters']['submit'] = array(
    '#type' => 'button',
    '#value' => t('Submit'),
  );
  $results = msm_get('collections', [
    'project' => ['name' => 1, 'databaseName' => 1, 'collectionName' => 1, 'type' => 1,
      'enabled' => 1, 'source' => 1, 'dependencies' => 1, 'locked' => 1,
    ],
    'sort' => ['weight' => 1, 'name' => 1, '_id' => 1],
  ], $server_id, $client_id);
  $form['server'] = array('#markup' => $values['client_server_id']);
  $table = array(
    '#theme' => 'table',
    '#header' => array(
      t('Name'),
      t('Database'),
      t('Collection'),
      t('Type'),
      t('Status'),
      t('Source Collection'),
      t('Dependencies'),
      t('Actions'),
    ),
    '#rows' => array(),
  );
  foreach ($results['collections'] AS $col) {
    $id = msm_client_server_collection_id($col['_id'], $server_id, $client_id);
    $actions = array(
      '#theme' => 'links',
      '#links' => array(),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    if (msm_collection_access($col, 'endpoint')) {
      $actions['#links']['endpoint'] = array(
        'href' => sprintf('admin/msm/col/%s/endpoint', $id),
        'title' => t('Endpoint'),
      );
    }
    if (msm_collection_access($col, 'settings')) {
      $actions['#links']['settings'] = array(
        'href' => sprintf('admin/msm/col/%s/settings', $id),
        'title' => t('Settings'),
      );
    }
    if (msm_collection_access($col, 'schema')) {
      $actions['#links']['schema'] = array(
        'href' => sprintf('admin/msm/col/%s/schema', $id),
        'title' => t('Schema'),
      );
    }
    if (msm_collection_access($col, 'eval')) {
      $actions['#links']['eval'] = array(
        'href' => sprintf('admin/msm/col/%s/custom', $id),
        'title' => t('Eval'),
      );
    }
    elseif (msm_collection_access($col, 'pipeline')) {
      $actions['#links']['pipeline'] = array(
        'href' => sprintf('admin/msm/col/%s/pipeline', $id),
        'title' => t('Pipeline'),
      );
    }
    elseif (msm_collection_access($col, 'mapReduce')) {
      $actions['#links']['map-reduce'] = array(
        'href' => sprintf('admin/msm/col/%s/map-reduce', $id),
        'title' => t('Map Reduce'),
      );
    }
    if (msm_collection_access($col, 'process')) {
      $actions['#links']['process'] = array(
        'href' => sprintf('admin/msm/col/%s/process', $id),
        'title' => t('Process'),
      );
    }
    if (msm_collection_access($col, 'delete')) {
      $actions['#links']['delete'] = array(
        'href' => sprintf('admin/msm/col/%s/delete', $id),
        'title' => t('Delete'),
      );
    }
    $dependencies = sprintf('<em>%s</em>', t('None'));
    if (!empty($col['dependencies'])) {
      $dependencies = array(
        '#theme' => 'links',
        '#links' => array(),
      );
      foreach ($col['dependencies'] AS $dep) {
        $dependencies['#links'][] = array(
          'title' => $dep,
          'href' => sprintf('admin/msm/col/%s', $dep),
        );
      }
      $dependencies = render($dependencies);
    }
    $source = !empty($col['source']) ? l($col['source'], sprintf('admin/msm/col/%s', $col['source'])) : 'n/a';
    $table['#rows'][] = array(
      l($col['name'], sprintf('admin/msm/col/%s', $id)),
      l($col['databaseName'], sprintf('admin/msm/db/%s', $col['databaseName'])),
      $col['collectionName'],
      empty($col['type']) ? 'n/a' : $col['type'],
      $col['enabled'] ? t('Enabled') : t('Disabled'),
      $source,
      $dependencies,
      render($actions),
    );
    $form['list'] = array(
      '#markup' => render($table),
    );
  }
  return $form;
}

function msm_collection_list_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_view(array $collection, array $options = array()) {
  dsm($collection);
  $options += array(
    'info' => TRUE,
    'schema_info' => TRUE,
    'schema' => TRUE,
    'test_result' => TRUE,
  );
  $module_path = drupal_get_path('module', 'msm');
  $build = array(
    '#theme' => 'msm_collection_view',
  );
  $has_schema = !empty($collection['schema']);
  if ($options['info']) {
    $count = msm_get(sprintf('collection/%s/count', $collection['_id']));
    if ($count['ok']) {
        $count = $count['count'];
    }
    else $count = 'err';

    $build['#info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Collection details'),
      '#items' => array(
        '_id' => array(t('Collection ID'), $collection['_id']),
        'name' => array(t('Name/Label'), $collection['name']),
        'collectionName' => array(t('Collection Name'), $collection['collectionName']),
        'databaseName' => array(t('Database'), l($collection['databaseName'], 'admin/msm/db/' . $collection['databaseName'])),
        'type' => array(t('Collection Type'), isset($collection['type']) ? $collection['type'] : null),
        'schema' => array(t('Has Schema'), $has_schema ? t('Yes') : t('No')),
        'totalDocuments' => array(t('Total Documents'), number_format($count, 0)),
      ),
    );
    if (!empty($collection['source'])) {
      $build['#info']['#items'][] = array(t('Source Collection ID'), l($collection['source'], 'admin/msm/col/' . $collection['source']));
    }
    if (!empty($collection['dependencies'])) {
      $deplinks = [];
      foreach ($collection['dependencies'] AS $dep) {
        $deplinks[] = array(
          'href' => sprintf('admin/msm/col/%s', $dep),
          'title' => $dep,
        );
      }
      $build['#info']['#items']['dependencies'] = array(
        t('Dependencies: (@count)', array('@count' => count($collection['dependencies']))),
        theme('links', array('links' => $deplinks))
      );
    }
  }
  if (!empty($collection['lastProcessed'])) {
    $build['#info']['#items']['lastProcessed'] = array(t('Last processed:'), $collection['lastProcessed']);
    $build['#info']['#items']['timesProcessed'] = array(t('Times processed:'), $collection['timesProcessed']);
    $build['#info']['#items']['millisProcessed'] = array(t('Milliseconds spent processing:'), number_format($collection['millisProcessed']));
    $build['#info']['#items']['averageSeconds'] = array(t('Average processing time:'), number_format($collection['millisProcessed']/$collection['timesProcessed']/1000, 2) . ' seconds');
  }
  if ($options['schema_info'] && $has_schema) {
    $fields = msm_schema_flatten_fields($collection['schema'], MSM_SCHEMA_FLATTEN_ALL);
    $build['#schema_info'] = array(
      '#theme' => 'msm_info',
      '#title' => t('Schema breakdown'),
      '#items' => array(
        'total_fields' => array(t('Total fields'), count($fields)),
      ),
    );
    $unused_fields = array();
    $counts = [];
    foreach ($fields AS $field_name => $field) {
      if (isset($field['percentContaining']) && $field['percentContaining'] <= 0) {
        $unused_fields[] = $field_name;
      }
      $type = $field['type'];
      if (!isset($counts[$type])) $counts[$type] = 0;
      $counts[$type]++;
    }
    foreach ($counts AS $type => $count) {
      $build['#schema_info']['#items'][] = array(ucfirst($type), $count);
    }
    if ($unused_fields) {
      $build['#schema_info']['#items'][] = array(t('Unused fields'), theme('item_list', array('items' => $unused_fields)));
    }
  }
  if ($options['schema']) {
    if (!empty($collection['schema'])) {
      $build['#schema'] = array(
        '#prefix' => sprintf('<h3>%s</h3>', t('Schema')),
        '#theme' => 'msm_schema_table',
        '#schema' => msm_schema_sort($collection['schema']),
        '#flatten' => TRUE,
      );
    }
    else {
      $build['#schema'] = array(
        '#markup' => t('No schema exists yet.'),
      );
    }
  }
  if ($options['test_result']) {
    if ($test = msm_collection_test_load($collection['_id'])) {
      $build['#test_result'] = array(
        '#theme' => 'msm_info',
        '#title' => t('Test Result'),
        '#items' => array(
          'result' => array(t('Result'), $test['pass'] ? t('Pass') : t('FAIL')),
          'last' => array(t('Last test ran on'), $test['last']),
        ),
      );
      if (!$test['pass']) {
        $build['#test_result']['#items']['error'] = array(t('Error'), $test['error']);
      }
    }
  }
  return $build;
}

function msm_collection_schema_view(array $collection) {
  if (!empty($collection['schema'])) {
    $build['schema'] = array(
      '#theme' => 'msm_schema_table',
      '#schema' => $collection['schema'],
      '#flatten' => TRUE,
    );
  }
  else {
    $build['schema'] = array(
      '#markup' => t('No schema exists yet.'),
    );
  }
  return $build;
}

function msm_collection_form($form, &$form_state, array $collection = array()) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $form['#is_new'] = empty($collection['_id']);
  $form_state += array(
    'collection_id' => empty($collection['_id']) ? NULL : $collection['_id'],
    'document' => NULL,
  );
  $ajax = array(
    'callback' => 'msm_collection_form_ajax',
    'wrapper' => $html_id,
  );
  $document = !empty($form_state['document']) ? $form_state['document'] : $collection;
  $document += array(
    'name' => null,
    'enabled' => FALSE,
    'persist' => 1,
    'type' => 'custom',
    'source' => null,
    'databaseName' => variable_get('msm_db_default', 'test'),
    'collectionName' => null,
    'description' => null,
    'client_server_id' => 'default:default',
  );
  list($client_id, $server_id) = explode(':', $document['client_server_id']);
  // "has (collection) source"
  $hasSource = in_array($document['type'], array('mapReduce', 'aggregation', 'custom'));
  $form['client_server_id'] = array(
    '#title' => t('Client\Server'),
    '#type' => 'select',
    '#options' => msm_clients_server_options(),
    '#default_value' => $document['client_server_id'],
    '#ajax' => $ajax,
  );
  $form['collection_id'] = array(
    '#type' => 'item',
    '#title' => t('Collection ID') . ':',
    '#markup' => $form_state['collection_id'],
    '#attributes' => array('class' => array('container-inline')),
    '#access' => !empty($form_state['collection_id']),
  );
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#weight' => 0,
    '#default_value' => $document['name'],
  );
  $form['description'] = array(
    '#title' => t('Description'),
    '#type' => 'textarea',
    '#weight' => 5,
    '#default_value' => $document['description'],
  );
  $form['type'] = array(
    '#title' => t('Type'),
    '#type' => 'select',
    '#options' => msm_collection_type_options($document['persist']),
    '#required' => TRUE,
    '#default_value' => $document['type'],
    '#ajax' => $ajax,
    '#weight' => 1000,
  );
  $form['persist'] = array(
    '#title' => t('Storage type'),
    '#type' => 'radios',
    '#options' => array(
      1 => t('Normal (persist)'),
      0 => t('Real-time Query Container (no storage)'),
    ),
    '#default_value' => $document['persist'] ? 1 : 0,
    '#access' => in_array($document['type'], array('aggregation', 'custom')),
    '#ajax' => $ajax,
    '#required' => TRUE,
    '#weight' => 1100,
  );
  $form['databaseName'] = array(
    '#title' => t('Database'),
    '#type' => 'select',
    '#options' => msm_database_options(),
//    '#default_value' => $form['#source']['database'],
    '#disabled' => !$form['#is_new'] || !in_array($document['type'], array('mapReduce', 'custom', 'import')),
    '#default_value' => $document['databaseName'],
    '#required' => TRUE,
    '#weight' => 2000,
  );
  $form['collectionName'] = array(
    '#title' => t('Collection name'),
    '#type' => 'textfield',
    '#required' => TRUE,
    '#weight' => 3000,
    '#default_value' => $document['collectionName'],
    '#disabled' => !$form['#is_new'],
  );
  $form['source'] = array(
    '#title' => t('Source collection'),
    '#type' => 'select',
    '#options' => array('') + msm_collection_options($server_id, $client_id, TRUE),
    '#access' => !in_array($document['type'], array('existing', 'import')),
    '#default_value' => $document['source'],
    '#ajax' => $ajax,
    '#required' => TRUE,
    '#weight' => 4000,
  );
  if (!empty($document['source'])) {
    if (empty($form['#source']) || $form['#source']['_id'] != $document['source']) {
      $form['#source'] = msm_get(sprintf('collection/%s', $document['source']));
    }
  }

  switch ($document['type']) {
    case 'existing':
      module_load_include('inc', 'msm', 'includes/database');
      $database = msm_database_load($document['databaseName']);
      $form['collectionName']['#type'] = 'select';
      $form['collectionName']['#options'] = msm_database_uninitialized_collection_options($database);
      break;
    case 'custom':
      break;
    case 'mapReduce':
      break;
  }

  $form['actions'] = array('#type' => 'actions', '#weight' => 10000);
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  return $form;
}

function msm_collection_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
  $document =& $form_state['document'];
  $skip = array('form_id', 'form_build_id', 'form_token', 'op', 'submit', 'destination');
  foreach (array_keys($values) AS $key) {
    if (in_array($key, $skip)) continue;
    $document[$key] = $values[$key];
  }
  $document['persist'] = intval($document['persist']) ? true : false;
  if (empty($document['source'])) unset($document['source']);
  if ($form['#is_new'] && empty($document['_id']) && !empty($document['databaseName']) && !empty($document['collectionName'])) {
    $collection_id = sprintf('%s:%s', $document['databaseName'], $document['collectionName']);
    $result = msm_get('collections', array('limit' => 1, 'query' => array('_id' => $collection_id)));
    if (!$result['total']) {
      $form_state['collection_id'] = $collection_id;
    }
    else {
      $parents = implode('][', $form['collectionName']['#parents']);
      form_set_error($parents, t('Collection ID (@id) is already in use.', array('@id' => $collection_id)));
    }
  }
}

function msm_collection_form_submit($form, &$form_state) {
  $document = $form_state['document'];
  list($client_id, $server_id) = explode(':', $document['client_server_id']);
  unset($document['client_server_id']);
  dsm($document);
  $id = msm_client_server_collection_id($form_state['collection_id'], $server_id, $client_id);
  if ($form['#is_new']) {
    $result = msm_put(sprintf('admin/collection/%s', $form_state['collection_id']), $document, $server_id, $client_id);
  }
  else {
    //$result = msm_post(sprintf('admin/collection/%s', $form_state['collection_id']), ['$set' => $document], $server_id, $client_id);
    $result = msm_post(sprintf('admin/collection/%s', $form_state['collection_id']), $document, $server_id, $client_id);
  }
  if (!$result['ok']) {
    $result = (array)$result;
    $result += array('error' => t('No further information to display.'));
    form_set_error('', t('An error occurred while attempting to save the collection: @err.', array('@err' => $result['error'])));
  }
  else $form_state['redirect'] = sprintf('admin/msm/col/%s', $id);
}

function msm_collection_clone_form($form, &$form_state, array $collection) {
  $form_state['original_collection_id'] = $collection['_id'];
  $cloned = $collection;
  unset($cloned['_id']);
  unset($cloned['collectionName']);
  $cloned['name'] = sprintf('Clone of %s', $collection['name']);
  $values = empty($form_state['clone']) ? array() : $form_state['clone'];
  $form['clone'] = msm_collection_form($form, $values, $cloned);
  unset($form['clone']['actions']);
  $form['clone'] += array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
  );
  $form['options'] = array('#tree' => TRUE);
  $form['options']['delete'] = array(
    '#type' => 'checkbox',
    '#title' => t('Delete this collection after cloning.'),
  );
  $form['options']['process'] = array(
    '#type' => 'checkbox',
    '#title' => t('Trigger processing on the newly cloned collection.'),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Clone collection'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/msm/col/' . $collection['_id'],
    '#title' => t('Cancel'),
  );
  return $form;
}

function msm_collection_clone_form_validate(&$form, &$form_state) {
  $values = array('values' => $form_state['values']['clone']);
  msm_collection_form_validate($form['clone'], $values);
  $form_state['clone'] = $values;
}


function msm_collection_clone_form_submit(&$form, &$form_state) {
  if (!$data = msm_request(sprintf('collection/%s', $form_state['original_collection_id']))) {
    form_set_error('', t('An error occurred.'));
  }
  unset($data['_id']);
  $data = array_merge($data, $form_state['clone']['document']);
  $result = msm_post(sprintf('collection/create/%s', $form_state['clone']['collection_id']), $data);
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s', $form_state['clone']['collection_id']);
  }
  else form_set_error('', t('An error has occurred: @error', array('@error' => $result['error'])));

  $options = $form_state['values']['options'];
  if ($options['delete']) {
    msm_request(sprintf('collection/%s/delete', $form_state['original_collection_id']));
  }
  if ($options['process']) {
    msm_request(sprintf('collection/%s/process', $form_state['clone']['collection_id']));
  }
}

function msm_collection_create_form($form, &$form_state) {
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = sprintf('</div>');
  $ajax = array(
    'callback' => 'msm_collection_create_form_ajax',
    'wrapper' => $html_id,
  );
  $document = !empty($form_state['document']) ? $form_state['document'] : array();
  $document += array(
    'enabled' => FALSE,
    'persist' => null,
    'type' => null,
    'source' => null,
  );
  $form['name'] = array(
    '#title' => t('Name'),
    '#type' => 'textfield',
    '#access' => !empty($document['source']) || $document['type'] == 'import',
    '#required' => TRUE,
    '#weight' => 95,
    '#ajax' => $ajax + array('event' => 'blur'),
  );
  $form['machine'] = array(
    '#title' => t('Machine'),
    '#type' => 'machine_name',
    '#machine_name' => array(
      'exists' => 'msm_collection_exists',
    ),
    '#access' => !empty($document['source']) || $document['type'] == 'import',
    '#required' => TRUE,
    '#weight' => 97,
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create collection'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/msm/col',
    '#title' => t('Cancel'),
  );
  $form['persist'] = array(
    '#title' => t('Storage type'),
    '#type' => 'radios',
    '#options' => array(
      1 => t('Normal (persist)'),
      0 => t('Real-time Query Container (no storage)'),
    ),
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['type'] = array(
    '#title' => t('Collection type'),
    '#type' => 'select',
    '#options' => msm_collection_type_options(),
    '#access' => $document['persist'] !== null,
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  $form['source'] = array(
    '#title' => t('Source collection'),
    '#type' => 'select',
    '#options' => array('') + msm_collection_options(),
    '#access' => !empty($document['type']) && $document['type'] != 'import',
    '#ajax' => $ajax,
    '#required' => TRUE,
  );
  if (!empty($document['source'])) {
    if (empty($form['#source']) || $form['#source']['_id'] != $document['source']) {
      $form['#source'] = msm_request(sprintf('collection/%s', $document['source']));
    }
  }
  if ($document['persist'] && !empty($document['source'])) {
    $document += array('destination' => array());
    $document['destination'] += array(
      'databaseName' => $form['#source']['databaseName'],
      'collectionName' => NULL,
    );
    $form['destination'] = array(
      '#tree' => TRUE,
      '#access' => !empty($document['source']) && $document['persist'],
    );
    if ($document['type'] == 'mapReduce') {
      $form['destination']['databaseName'] = array(
        '#title' => t('Destination database'),
        '#type' => 'select',
        '#options' => msm_database_options($document['server_id'], $document['client_id']),
        '#default_value' => $form['#source']['databaseName'],
        '#required' => TRUE,
      );
    }
    else {
        $form['destination']['databaseName'] = array(
          '#type' => 'value',
          '#value' => $document['destination']['databaseName'],
        );
    }
    $form['destination']['collectionName'] = array(
      '#title' => t('Destination collection'),
      '#type' => 'textfield',
      '#description' => t('Specify the name of the collection to create.'),
      '#required' => TRUE,
      '#ajax' => $ajax += array('event' => 'blur'),
    );
  }
  if (empty($form_state['id'])) {
    $form['actions']['submit']['#access'] = FALSE;
  }

  $form_state['document'] = $document;
  return $form;
}

function msm_collection_create_form_validate(&$form, &$form_state) {
  $values = $form_state['values'];
  $document =& $form_state['document'];
  $skip = array('form_id', 'form_build_id', 'form_token', 'machine', 'op', 'submit', 'destination');
  foreach (array_keys($values) AS $key) {
    if (in_array($key, $skip)) continue;
    $document[$key] = $values[$key];
  }
  $document['persist'] = intval($document['persist']) ? true : false;
  if (!$document['persist']) {
    $form_state['id'] = empty($values['machine']) ? null : $values['machine'];
    $document['collectionName'] = $form['#source']['collectionName'];
    $document['databaseName'] = $form['#source']['databaseName'];
  }
  else {
    if ($document['type'] == 'import') {

    }
    elseif (!empty($values['destination']['collectionName']) && !empty($values['destination']['databaseName'])) {
      $document['collectionName'] = $values['destination']['collectionName'];
      $document['databaseName'] = $values['destination']['databaseName'];
      $form_state['id'] = sprintf('%s.%s', $document['databaseName'], $document['collectionName']);
    }
    else $form_state['id'] = null;
  }
}

function msm_collection_create_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_create_form_submit($form, &$form_state) {
  $result = msm_post(sprintf('collection/create/%s', $form_state['id']), $form_state['document']);
  if ($result['ok']) {
    switch ($form_state['document']['type']) {
      case 'mapReduce';
        $step = 'map-reduce';
        break;
      case 'aggregation';
        $step = 'pipeline';
        break;
      case 'custom':
        $step = 'custom';
        break;
    }
    $form_state['redirect'] = sprintf('admin/msm/col/%s/%s', $form_state['id'], $step);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_delete_form($form, &$form_state, $collection) {
  $form['#collection'] = $collection;
  if ($result = msm_request(sprintf('admin/collection/%s/dependents', $collection['_id']))) {
    $form['dependents'] = array(
      '#type' => 'markup',
      '#markup' => theme('item_list', array(
        'items' => $result,
        'title' => t('Collections dependending on @name:', array('@name' => $collection['name'])),
      )),
    );
  }
  return confirm_form($form,
    t('Are you sure you want to delete all schema information for @id', array('@id' => $collection['_id'])),
    sprintf('admin/msm/col/%s', $collection['_id']),
    t('No user database will be deleted. Only schema information.'),
    t('Delete collection'),
    t('Cancel')
  );
}

function msm_collection_delete_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $result = msm_delete(sprintf('admin/collection/%s', $collection['_id']), array(), $collection['server_id'], $collection['client_id']);
  if ($result['ok']) {
    $form_state['redirect'] = 'admin/msm/col';
  }
}

function msm_collection_type_options($persistant = TRUE) {
  if ($persistant) {
    return array(
      'mapReduce' => t('Map Reduce'),
      'aggregation' => t('Aggregation'),
      'custom' => t('Custom'),
      'import' => t('Import'),
      'existing' => t('Existing'),
    );
  }
  return array(
    'aggregation' => t('Aggregation'),
    'custom' => t('Custom'),
  );
}

function msm_collection_exists() {
  $args = func_get_args();
  return false;
}

function msm_collection_settings_form($form, &$form_state, array $collection) {
  $collection = json_decode(json_encode($collection), TRUE);
  $collection += array(
    'preProcess' => NULL,
    'postProcess' => NULL,
    'preExecute' => NULL,
    'postExecute' => NULL,
    'type' => 'existing',
    'persist' => FALSE,
    'weight' => 0,
  );
  $weight_options = range(-300, 300);
  $form['weight'] = array(
      '#type' => 'select',
      '#title' => t('Weight'),
      '#required' => TRUE,
      '#options' => array_combine($weight_options, $weight_options),
      '#default_value' => $collection['weight'],
  );
  $form['process'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Processing'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preProcess']) && empty($collection['postProcess']),
    '#access' => $collection['persist'],
  );
  $form['process']['preProcess'] = array(
    '#title' => t('Pre-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $collection['preProcess'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['process']['postProcess'] = array(
    '#title' => t('Post-process code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postProcess'],
  );
  $form['execute'] = array(
    '#tree' => FALSE,
    '#type' => 'fieldset',
    '#title' => t('Execution'),
    '#collapsible' => TRUE,
    '#collapsed' => empty($collection['preExecute']) && empty($collection['postExecute']),
  );
  $form['execute']['preExecute'] = array(
    '#title' => t('Pre-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['preExecute'],
  );
  $form['execute']['postExecute'] = array(
    '#title' => t('Post-execute code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#default_value' => $collection['postExecute'],
  );
  $form['enabled'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enabled'),
    '#default_value' => $collection['enabled'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save'),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_settings_form_validate($form, &$form_state) {
}

function msm_collection_settings_form_submit($form, &$form_state) {
  $fields = array('preExecute', 'postExecute', 'preProcess', 'postProcess');
  $values = $form_state['values'];
  $update = array(
    '$set' => array(
      'enabled' => (bool)$values['enabled'],
      'weight' => (int)$values['weight'],
    ),
    '$unset' => array(),
  );
  $collectionId = $form['#collection']['_id'];
  foreach ($fields AS $f) {
    if (empty($values[$f])) {
      $update['$unset'][$f] = "";
    }
    else $update['$set'][$f] = $values[$f];
  }
  if (empty($update['$unset'])) {
    unset($update['$unset']);
  }
  $result = msm_post(sprintf('collection/%s/update', $collectionId), $update);
  if ($result['ok']) {
    $form_state['redirect'] = sprintf('admin/msm/col/%s/view', $collectionId);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_endpoint(array $collection) {
  return drupal_get_form('msm_collection_endpoint_form', $collection);
}

function msm_collection_endpoint_form($form, &$form_state, array $collection) {
  $params = empty($form_state['values']['params']) ? array() : $form_state['values']['params'];
  $params += array(
    'limit' => 1,
    'skip' => 0,
    'query' => null,
    'project' => null,
    'sort' => null,
    'options' => array(),
  );
  $params['options'] += array(
    'array_item_limit' => 3,
  );
  $result = msm_get(sprintf('collection/%s/query', $collection['_id']), $params);
  dsm($result);
  $html_id = drupal_html_class(__FUNCTION__);
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $ajax = array(
    'callback' => 'msm_collection_endpoint_form_ajax',
    'wrapper' => $html_id,
  );
  $form['params'] = array(
    '#type' => 'fieldset',
    '#title' => t('Parameters'),
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#tree' => TRUE,
  );
  $form['params']['query'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Query'),
    '#default_value' => $params['query'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['sort'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Sort'),
    '#default_value' => $params['sort'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['project'] = array(
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#title' => t('Project'),
    '#default_value' => $params['project'],
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
  );
  $form['params']['skip'] = array(
    '#type' => 'textfield',
    '#title' => t('Skip'),
    '#default_value' => $params['skip'],
  );
  $form['params']['update'] = array(
    '#type' => 'button',
    '#value' => t('Run query'),
    '#ajax' => $ajax,
  );
  $form['params']['options'] = array('#tree' => TRUE);
  $form['params']['options']['array_item_limit'] = array(
    '#title' => t('Array Item Limit'),
    '#type' => 'textfield',
    '#default_value' => $params['options']['array_item_limit'],
  );
dsm($params['options']);
  if (!empty($result['results'][0])) {
    $form['result'] = array(
      '#markup' => theme('msm_schema_result_tree', array(
        'schema' => $result['schema'],
        'result' => $result['results'][0],
        'array_item_limit' => (int)$params['options']['array_item_limit'],
      )),
    );
  }
  else {
    $form['empty'] = array(
      '#markup' => sprintf('<p>%s</p>', t('No results were found that matched your search.')),
    );
  }
  $form['total'] = array(
    '#type' => 'item',
    '#title' => t('Total documents:'),
    '#markup' => $result['totalResults'],
  );
  $form['actions'] = array(
    '#type' => 'actions',
    '#attributes' => array('class' => array('clearfix')),
  );
  if (((int)$params['skip']) < ($result['totalResults']-1)) {
    $form['actions']['next'] = array(
      '#type' => 'button',
      '#value' => t('Next'),
      '#attributes' => array('style' => 'float:right;'),
      '#ajax' => $ajax,
    );
  }
  if ($params['skip']) {
    $form['actions']['prev'] = array(
      '#type' => 'button',
      '#value' => t('Previous'),
      '#ajax' => $ajax,
    );
  }
  return $form;
}

function msm_collection_endpoint_form_validate(&$form, &$form_state) {
  $params = $form_state['values']['params'];
  $input = &$form_state['input']['params'];

  $ele = $form_state['triggering_element'];
  if ($ele['#parents'][0] == 'next') {
    $params['skip'] = ((int)$params['skip'])+1;
    $input['skip'] = $params['skip'];
  }
  elseif ($ele['#parents'][0] == 'prev') {
    $params['skip'] = ((int)$params['skip'])-1;
    $input['skip'] = $params['skip'];
  }
  $result = array();
  foreach ($params AS $k => $v) {
    if ($v && !empty($v) && $k != 'update') {
      $result[$k] = $v;
    }
  }
  foreach (array('project', 'sort', 'query') AS $k) {
    if (!empty($result[$k])) {
      if (!$result[$k] = json_decode($result[$k])) {
        form_set_error("params][$k", t('Invalid JSON detected.'));
      }
    }
  }
  $form_state['values']['params'] = $result;
}

function msm_collection_endpoint_form_ajax($form, &$form_state) {
  return $form;
}

function msm_collection_process_form($form, &$form_state, array $collection) {
  return $form;
}

function msm_collection_reset_form($form, &$form_state, array $collection) {
  $form['#collection'] = $collection;
  return confirm_form($form,
    t('Are you sure you want to reset the schema for @collection', array('@collection' => $collection['_id'])),
    sprintf('admin/msm/collection/%s', $collection['_id']),
    t('This will remove the schema information.'),
    t('Reset schema'),
    t('Cancel')
  );
}

function msm_collection_reset_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $update = array(
    '$set' => array('enabled' => false),
    '$unset' => array('schema' => '', 'schemaImported' => '', 'analyzingSchema' => ''),
  );
  $result = msm_post(sprintf('collection/%s/update', $collection['_id']), $update);
  if ($result['ok']) {
    drupal_set_message(t('The schema is being reset. Refresh the page momentarily as necessary.'));
    $form_state['redirect'] = sprintf('admin/msm/col/%s/schema/analyze', $collection['_id']);
  }
  else form_set_error('', t('An error has occured: @error', array('@error' => $result['error'])));
}

function msm_collection_schema_analyze_form($form, &$form_state, array $collection) {
  $result = msm_request(sprintf('collection/%s/count', $collection['_id']));
  $form['#attributes'] = array('class' => array('clearfix'));
  if (!$result['ok']) {
    $form['error'] = array(
      '#markup' => t('Failed to retrieve record count from @db -> @collection', array(
        '@db' => $form['#db'],
        '@col' => $form['#cname'],
      )),
    );
  }
  $form['totalRecords'] = array(
    '#type' => 'item',
    '#title' => t('Total records'),
    '#markup' => $result['count'],
  );
  $form['limit'] = array(
    '#type' => 'textfield',
    '#title' => t('Limit'),
    '#description' => t('Limit schema analysis to # records.'),
    '#default_value' => $result['count'],
  );
  drupal_set_title(t('Analyze and import schema for @collection', array('@collection' => $collection['_id'])));
  $form['actions'] = array(
    '#prefix' => sprintf('<p>%s</p>', t('This will analyze the existing data to derrive the schema structure and import it as an "existing" collection.')),
    '#type' => 'actions',
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Analyze & Import Schema'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => sprintf('admin/msm/col/%s', $collection['_id']),
  );
  $form['#collection'] = $collection;
  return $form;
}

function msm_collection_schema_analyze_form_submit($form, &$form_state) {
  $collection = $form['#collection'];
  $options = array('limit' => $form_state['values']['limit']);
  $result = msm_post(sprintf('collection/%s/analyze', $collection['_id']), $options);
  $form_state['redirect'] = sprintf('admin/msm/col/%s/schema/view', $collection['_id']);
  drupal_set_message(t('The schema is being analyzed. Refresh the page momentarily as necessary.'));
}

function msm_collection_tests_form($form, &$form_state, $collection) {
  $form['#collection_id'] = $collection['_id'];
  if (empty($form_state['values']['test'])) {
    try {
      $test = msm_get(sprintf('collection/%s/test', $collection['_id']));
    }
    catch (Exception $e) {
      $test = array();
    }
  }
  else {
    $test = $form_state['values']['test'];
  }
  $test += array(
    'collectionName' => $collection['collectionName'],
    'databaseName' => $collection['databaseName'],
    'units' => array(),
  );
  $test['units'] += array(0 => array());
  $test['units'][0] += array(
    'code' => "let count = await col.count();\nassert(count, 'Collection is empty');",
  );

  if ($test['last']) {
    $form['result'] = array(
      '#type' => 'markup',
      '#markup' => theme('table', array(
        'header' => array(
          t('Result'),
          t('Error'),
          t('Last test ran'),
        ),
        'rows' => array(array(
          $test['pass'] ? t('Pass') : t('FAIL'),
          empty($test['error']) ? '-' : $test['error'],
          $test['last'],
        )),
      )),
    );
  }

  $form['test'] = array('#tree' => TRUE);
  $form['test']['_id'] = array(
    '#type' => 'value',
    '#value' => $collection['_id'],
  );
  $form['test']['collectionName'] = array(
    '#type' => 'value',
    '#value' => $test['collectionName'],
  );
  $form['test']['databaseName'] = array(
    '#type' => 'value',
    '#value' => $test['databaseName'],
  );
  $form['test']['code'] = array(
    '#title' => t('Code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#required' => TRUE,
    '#rows' => 20,
    '#default_value' => $test['units'][0]['code'],
  );
  $form['options'] = array('#tree' => TRUE);
  $form['options']['run_test'] = array(
    '#type' => 'checkbox',
    '#title' => t('Run test after saving.'),
    '#default_value' => 1,
  );
  $form['options']['lock'] = array(
    '#type' => 'checkbox',
    '#title' => t('Lock collection after passing test.'),
    '#default_value' => !empty($collection['locked']),
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => sprintf('admin/msm/col/%s', $collection['_id']),
    '#title' => t('Cancel'),
  );
  return $form;
}

function msm_collection_tests_form_submit($form, &$form_state) {
  $test = $form_state['values']['test'];
  $test['units'] = array(array(
    'description' => 'Collection Base Tests',
    'code' => $test['code'],
  ));
  unset($test['code']);
  $result = msm_post(sprintf('colelction/%s/test', $test['_id']), $test);
  if ($result['ok']) {
    $options = $form_state['values']['options'];
    if ($options['run_test']) {
      $testresult = msm_get(sprintf('collection/%s/run-test', $test['_id']));
      if ($testresult['result']['pass'] && $options['lock']) {
        $res2 = msm_post(sprintf('collection/%s/update', $form['#collection_id']), [
          '$set' => ['locked' => TRUE]
        ]);
        if (!$res2['ok']) {
          form_set_error('', t('An error has occured: @error', array('@error' => $res2['error'])));
        }
      }
    }
  }
}

function msm_collection_test_load($collection_id) {
  try {
    return msm_request(sprintf('collection/%s/test', $collection_id));
  }
  catch (Exception $e) {
    return null;
  }
}

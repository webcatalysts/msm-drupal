<?php


function msm_eval_form($form, &$form_state, array $collection) {
  $collection += array(
    'dependencies' => array(),
  );
  if (!empty($collection['source'])) {
    $source = msm_request(sprintf('collection/%s', $collection['source']));
  }
  $form['actions'] = array(
    '#type' => 'actions',
    '#weight' => 100,
  );
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Continue'),
  );
  $form['actions']['save_and_process'] = array(
    '#type' => 'submit',
    '#value' => t('Save and Process'),
  );
  $form['eval'] = array(
    '#tree' => TRUE,
    '#type' => 'fieldset',
    '#title' => t('Custom evaluation'),
  );
  $form['eval']['code'] = array(
    '#title' => t('Code'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#after_build' => array('msm_text_format_after_build'),
    '#element_validate' => array('msm_text_format_validate'),
    '#required' => TRUE,
    '#rows' => 20,
    '#default_value' => !empty($collection['eval']['code']) ? $collection['eval']['code'] : null,
  );
  $form['deps'] = array(
    '#type' => 'fieldset',
    '#title' => t('Dependencies (@count)', array('@count' => count($collection['dependencies']))),
    '#collapsible' => TRUE,
    '#collapsed' => FALSE,
  );
  $options = msm_collection_options();
  if (isset($options[$collection['_id']])) {
    unset($options[$collection['_id']]);
  }
  if (isset($options[$collection['source']])) {
      unset($options[$collection['source']]);
  }
  $form['deps']['dependencies'] = array(
    '#type' => 'checkboxes',
    '#type' => 'select',
    '#multiple' => TRUE,
    '#chosen' => TRUE,
    '#options' => $options,
    '#default_value' => $collection['dependencies'],
  );
  $form['options'] = array('#tree' => TRUE);
  $form['options']['increment_version'] = array(
    '#type' => 'checkbox',
    '#title' => t('Increment Schema Version'),
  );
  $form['#collection'] = $collection;

  $help = array();
  $help[] = array(
    'label' => 'callback',
    'value' => 'function',
  );
  if (!empty($collection['source'])) {
    $help[] = array(
      'label' => 'source',
      'value' => l($source['collection'], sprintf('admin/msm/col/%s', $collection['source'])),
      'description' => t('The source collection object.'),
    );
    $help[] = array(
      'label' => 'sourceDatabase',
      'value' => l($source['database'], sprintf('admin/msm/db/%s', $source['database'])),
      'description' => t('The source database object.'),
      'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Db.html',
    );
    $help[] = array(
      'label' => 'sourceDatabaseName',
      'value' => $source['database'],
      'The source database name (string).',
    );
    $help[] = array(
      'label' => 'sourceCollection',
      'value' => $source['collection'],
      'description' => t('The source collection object.'),
      'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Collection.html',
    );
    $help[] = array(
      'label' => 'sourceCollectionName',
      'value' => $source['collection'],
      'description' => t('The source collection name (string).'),
    );
  }
  $help[] = array(
    'label' => 'destinationDatabase',
    'value' => l($collection['database'], sprintf('admin/msm/db/%s', $collection['database'])),
    'description' => t('The destination database object.'),
    'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Db.html',
  );
  $help[] = array(
    'label' => 'destinationDatabaseName',
    'value' => $collection['database'],
    'description' => t('The destination database name (string).'),
  );
  $help[] = array(
    'label' => 'destinationCollection',
    'value' => $collection['collection'],
    'description' => t('The destination collection object.'),
    'documentation_url' => 'http://mongodb.github.io/node-mongodb-native/2.2/api/Collection.html',
  );
  $help[] = array(
    'label' => 'destinationCollectionName',
    'value' => $collection['collection'],
    'description' => t('The destination collection name (string).'),
  );
  $form['help'] = array(
    '#type' => 'fieldset',
    '#collapsible' => TRUE,
    '#collapsed' => TRUE,
    '#title' => t('Help'),
    'variables' => array(
      '#markup' => theme('msm_variables_help', array('title' => t('Variables'), 'items' => $help)),
    ),
    '#weight' => -10,
  );
  return $form;
}

function msm_eval_form_submit($form, &$form_state) {
  $collectionId = $form['#collection']['_id'];
  $ele = $form_state['triggering_element'];
  $path = $ele['#value'] == t('Save and Process') ? 'collection/%s/process' : 'collection/%s/update';
  $update = array(
    '$set' => array(
      'eval' => $form_state['values']['eval'],
    ),
  );
  if ($dependencies = array_filter($form_state['values']['dependencies'])) {
    $update['$set']['dependencies'] = array_keys($dependencies);
  }
  else { $update['$unset'] = array('dependencies' => ''); }
  if ($form_state['values']['options']['increment_version']) {
    $update['$set']['version'] = md5($update['$set']['eval']['code']);
  }
  $result = msm_post(sprintf($path, $collectionId), $update);

  if ($result['ok']) {
    if ($form_state['triggering_element']['#value'] != t('Save and Process')) {
      $form_state['redirect'] = sprintf('admin/msm/col/%s/schema', $collectionId);
    }
  }
  else form_set_error('', t('An error has occured while attempting to update the collection: @error', array('@error' => $result['error'])));
}

<?php

function msm_database_list() {
  $build = array();
  $databases = msm_request('databases');
  $build['existing'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Database name'),
      t('Size on disk'),
      t('Empty'),
      t('Actions'),
    ),
    '#rows' => [],
  );
  foreach ($databases AS $dbinfo) {
    $actions = array(
      '#theme' => 'links',
      '#links' => array(),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    if ($dbinfo['enabled']) {
      $actions['#links'][] = array(
        'href' => 'admin/structure/msm/db/' . $dbinfo['name'] . '/edit',
        'title' => t('Edit'),
      );
      $actions['#links'][] = array(
        'href' => 'admin/structure/msm/db/' . $dbinfo['name'] . '/disable',
        'title' => t('Disable'),
      );
      $actions['#links'][] = array(
        'href' => sprintf('admin/structure/msm/db/%s/delete', $dbinfo['name']),
        'title' => t('Delete'),
      );
    }
    else {
      $actions['#links'][] = array(
        'href' => 'admin/structure/msm/db/enable/' . $dbinfo['name'],
        'title' => t('Enable'),
      );
    }

    $build['existing']['#rows'][] = array(
      $dbinfo['enabled'] ? l($dbinfo['name'], sprintf('admin/structure/msm/db/%s', $dbinfo['name'])) : $dbinfo['name'],
      format_size($dbinfo['sizeOnDisk']),
      $dbinfo['empty'] ? t('Yes') : t('No'),
      render($actions),
    );
  }
  return $build;
}

function msm_database_view($db) {
  module_load_include('inc', 'msm', 'includes/collections');
  module_load_include('inc', 'msm', 'includes/containers');
  $build = array();
  $rows = array();
  dsm($db);
  foreach ($db['collections'] AS $col) {
    $actions = array(
      '#theme' => 'links',
      '#links' => array(),
      '#attributes' => array('class' => array('links', 'inline')),
    );
    if (isset($col['schemaImported']) || isset($col['analyzingSchema'])) {
      $actions['#links']['reset_schema'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/reset', $col['_id']),
        'title' => t('Reset Schema'),
      );
    }
    if (!empty($col['type']) || $col['enabled']) {
      $actions['#links']['manage_schema'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/schema', $col['_id']),
        'title' => t('Manage Schema'),
      );
      $actions['#links']['delete'] = array(
        'href' => sprintf('admin/structure/msm/col/%s/delete', $col['_id']),
        'title' => t('Delete'),
      );
    }
    else {
      if (empty($col['type']) && !isset($col['analyzingSchema'])) {
        $actions['#links']['analyze_schema'] = array(
          'href' => sprintf('admin/structure/msm/db/%s/analyze-schema/%s', $db['name'], $col['collection']),
          'title' => t('Analyze & Import Schema'),
        );
      }
    }
    $rows[] = array(
      $col['name'],
      empty($col['type']) ? t('Pre-existing') : ucfirst($col['type']),
      render($actions),
    );
  }

  $build['collections'] = array(
    '#theme' => 'table',
    '#header' => array(
      t('Collection name'),
      t('Type'),
      t('Actions'),
    ),
    '#rows' => $rows,
  );
  return $build;
}

function msm_database_add_form($form, &$form_state) {
  form_load_include($form_state, 'inc', 'msm', 'includes/database');
  $form['name'] = array(
    '#title' => t('Database name'),
    '#type' => 'textfield',
    '#required' => TRUE,
  );
  $weights = range(-25, 25);
  $form['weight'] = array(
    '#title' => t('Weight'),
    '#type' => 'select',
    '#options' => array_combine($weights, $weights),
    '#default_value' => 0,
  );
  $form['submit'] = array(
    '#value' => t('Create database'),
    '#type' => 'submit',
  );
  return $form;
}

function msm_database_add_form_validate(&$form, &$form_state) {
  $databases = msm_request('databases');
  foreach ($databases AS $dbinfo) {
    if ($dbinfo['name'] == $form_state['values']['name']) {
      form_set_error('name', t('A database with that name already exists.'));
    }
  }
}

function msm_database_add_form_submit($form, &$form_state) {
  $result = msm_request(sprintf('database/%s/enable', check_plain($form_state['values']['name'])));
  if ($result['ok'] === 1) {
    $form_state['redirect'] = sprintf('admin/structure/msm/db/%s', $form_state['values']['name']);
  }
  form_set_error(null, t('An error occurred while attempting to enable the @name database', array('@name' => $form_state['values']['name'])));
}

function msm_database_enable($name) {
  $result = msm_request('database/' . check_plain($name) . '/enable');
  drupal_goto('admin/structure/msm/db');
}

function msm_database_disable($db) {
  $result = msm_request('database/' . $db['name'] . '/disable');
  drupal_goto('admin/structure/msm/db');
}

function msm_database_delete_form($form, &$form_state, $db) {
  $form_state['db'] = $db;
  if ($result = msm_request(sprintf('collections/%s', $db['name']))) {
    $form['collections'] = array(
      '#prefix' => sprintf('<h3>%s</h3><p>%s</p><ul>',
        t('Cascase Collections'),
        t('The following collection metadata will also be removed.')
      ),
      '#suffix' => '</ul>',
    );
    foreach ($result AS $col) {
      $form['collections'][$col['_id']] = array(
        '#type' => 'link',
        '#prefix' => '<li>',
        '#suffix' => '</li>',
        '#href' => sprintf('admin/structure/msm/col/%s', $col['_id']),
        '#title' => $col['name'],
      );
    }
  }
  return confirm_form($form,
    t('Are you sure you want to delete %name?', array('%name' => $db['name'])),
    'admin/structure/msm/db/' . $db['name'],
    t('Only associated schema information will be removed, no user data will be deleted.'),
    t('Remove Database Schema'),
    t('Cancel')
  );
}

function msm_database_delete_form_submit($form, &$form_state) {
  $result = msm_request(sprintf('database/%s/delete', $form_state['db']['name']));
  $form_state['redirect'] = 'admin/structure/msm/db';
}

function msm_database_analyze_schema_form($form, &$form_state, $db, $cname) {
  $form['#db'] = $db;
  $form['#cname'] = $cname;
  $result = msm_request(sprintf('database/%s/count/%s', $form['#db']['name'], $form['#cname']));
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

  drupal_set_title(t('Analyze and import schema for @collection', array('@collection' => $cname)));
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
    '#href' => sprintf('admin/structure/msm/db/%s', $form['#db']['name']),
  );
  return $form;
}

function msm_database_analyze_schema_form_submit($form, &$form_state) {
  $options = array('limit' => $form_state['values']['limit']);
  $result = msm_post(sprintf('database/%s/analyze/%s', $form['#db']['name'], $form['#cname']), $options);
  dsm($result);
  $form_state['redirect'] = sprintf('admin/structure/msm/db/%s', $form['#db']['name']);
}

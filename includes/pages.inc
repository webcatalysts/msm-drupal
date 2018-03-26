<?php

function msm_status_page() {
  return drupal_get_form('msm_status_form');
}

function msm_status_form($form, &$form_state) {
  $values = empty($form_state['values']['filters']) ? array() : $form_state['values']['filters'];
  $values += array(
    'client_server_id' => 'default:default',
  );
  list($client_id, $server_id) = explode(':', $values['client_server_id']);
  $html_id = drupal_html_id(__FUNCTION__);
  $ajax = array(
    'callback' => 'msm_status_form_ajax',
    'wrapper' => $html_id,
  );
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['client_server_id'] = array(
    '#title' => t('Client/Server'),
    '#type' => 'select',
    '#options' => msm_clients_server_options(),
    '#default_value' => $values['client_server_id'],
    '#ajax' => $ajax,
  );
  $form['filters']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $result = msm_get('status', array(), $server_id, $client_id);
  $table = array();
  $header = array(t('Service'), t('Status'));
  $rows = array();
  if ($result === NULL) {
    $rows[0] = array(t('API Server'), t('Not responding'));
    $rows[1] = array(t('Database Server'), t('Unknown'));
  }
  elseif($result['ok'] !== 1) {
    $rows[0] = array(t('API Server'), t('Not responding'));
    $rows[1] = array(t('Database Server'), t('Unknown'));
  }
  else {
    $rows[0] = array(t('Express API Server'), t('Running'));
    $rows[1] = array(t('MongoDB Database Server'), t('Running'));
    $rows[0][] = variable_get('msm_host', 'http://127.0.0.1:3000');
    $rows[1][] = sprintf('%s (version %s)', $result['host'], $result['version']);
    $header[] = t('Host');
    dsm($result);
    $table['database'] = array(
      '#prefix' => sprintf('<h3>%s</h3>', t('Database Info')),
      '#theme' => 'table',
      '#rows' => array(
        array('Host:', $result['host']),
        array('Version:', $result['version']),
        array('Process:', $result['process']),
        array('Pid:', $result['pid']),
        array('Uptime:', $result['uptime']),
        array('Local time:', $result['localTime'])
      ),
    );
    $table['connections'] = array(
      '#prefix' => sprintf('<h3>%s</h3>', t('Database Connections')),
      '#theme' => 'table',
      '#rows' => array(
        array('Current:', $result['connections']['current']),
        array('Available:', $result['connections']['available']),
        array('Total created:', $result['connections']['totalCreated']),
      ),
    );
  }
  $table['status'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#weight' => -100,
  );
  $form['status'] = array('#markup' => drupal_render($table));
  return $form;
}

function msm_status_form_ajax($form, &$form_state) {
  return $form;
}

function msm_settings_page($form, &$form_state) {
}

function msm_settings_filters($form, &$form_state) {
  $filters = empty($form_state['values']['filters']) ? array() : $form_state['values']['filters'];
  $filters += array(
    'client_server_id' => 'default:default',
  );
  list($client_id, $server_id) = explode(':', $filters['client_server_id']);
  $html_id = drupal_html_id(__FUNCTION__);
  $ajax = array(
    'callback' => 'msm_status_form_ajax',
    'wrapper' => $html_id,
  );
  $form['#prefix'] = sprintf('<div id="%s">', $html_id);
  $form['#suffix'] = '</div>';
  $form['filters'] = array(
    '#type' => 'fieldset',
    '#tree' => TRUE,
    '#attributes' => array('class' => array('container-inline')),
  );
  $form['filters']['client_server_id'] = array(
    '#title' => t('Client/Server'),
    '#type' => 'select',
    '#options' => msm_clients_server_options(),
    '#default_value' => $filters['client_server_id'],
    '#ajax' => $ajax,
  );
  $form['filters']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
}

function msm_settings_form($form, &$form_state, $server_id = 'default', $client_id = 'default') {
  if (!empty($form_state['values']['settings'])) {
    $settings = $form_state['values']['settings'];
  }
  else {
    $settings = msm_get('settings', array(), $server_id, $client_id);
  }
  $form['settings'] = array(
    '#tree' => TRUE,
  );
  $settings += array(
    'evalonboot' => NULL,
    'collectionpreprocess' => NULL,
    'collectionpostprocess' => NULL,
    'collectionpreexecute' => NULL,
    'collectionpostexecute' => NULL,
  );
  $form['settings']['evalonboot'] = array(
    '#title' => t('On Bootup'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $settings['evalonboot'],
    '#element_validate' => array('msm_text_format_validate'),
    '#after_build' => array('msm_text_format_after_build'),
    '#rows' => 30,
  );
  $form['settings']['collectionpreprocess'] = array(
    '#title' => t('Collection PreProcess'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $settings['collectionpreprocess'],
    '#element_validate' => array('msm_text_format_validate'),
    '#after_build' => array('msm_text_format_after_build'),
    '#rows' => 30,
  );
  $form['settings']['collectionpostprocess'] = array(
    '#title' => t('Collection PostProcess'),
    '#type' => 'text_format',
    '#format' => 'catalyst_js',
    '#default_value' => $settings['collectionpostprocess'],
    '#element_validate' => array('msm_text_format_validate'),
    '#after_build' => array('msm_text_format_after_build'),
    '#rows' => 30,
  );
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Update System Settings'),
  );
  return $form;
}

function msm_settings_form_ajax($form, &$form_state) {
  return $form;
}

function msm_settings_form_submit($form, &$form_state) {
  $result = msm_post('admin/settings', $form_state['values']['settings']);
  sleep(1);
}

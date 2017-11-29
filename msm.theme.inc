<?php


function theme_msm_schema_table($variables) {
  $header = array(
    t('Field'),
    t('Title'),
    t('Description'),
    t('Priority'),
    t('Enabled'),
  );
  $rows = array();
  $fields = msm_schema_flatten_fields($variables['schema'], MSM_SCHEMA_FLATTEN_ALL);
  foreach ($fields AS $f => $fi) {
    $fi += array(
      'display' => array(),
      'enabled' => TRUE,
    );
    $fi['display'] += array(
      'title' => sprintf('<em>%s</em>', t('Not defined')),
      'description' => sprintf('<em>%s</em>', t('Not defined')),
      'priority' => sprintf('<em>%s</em>', t('Not defined')),
    );
    $row = array();
    $row[] = $f;
    $row[] = $fi['display']['title'];
    $row[] = $fi['display']['description'];
    $row[] = $fi['display']['priority'];
    $row[] = $fi['enabled'] ? t('Enabled') : t('Disabled');
    $rows[] = $row;
  }
  return theme('table', array('header' => $header, 'rows' => $rows));
}

function theme_msm_project_schema_table(&$variables) {
  $form = &$variables['form'];
  $header = array(
    t('Parent'),
    t('Local Name'),
    t('Weight'),
    t('Actions'),
  );
  $rows = array();
  foreach (element_children($form) AS $i) {
    $rows[] = array(
      drupal_render($form[$i]['parent']),
      drupal_render($form[$i]['localName']),
      drupal_render($form[$i]['weight']),
      drupal_render($form[$i]['actions']) . drupal_render_children($form[$i]),
    );
  }
  return theme('table', array('header' => $header, 'rows' => $rows)) . drupal_render_children($variables['form']);
}

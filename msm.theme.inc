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

function theme_msm_variables_help($variables) {
  $header = array(
    t('Variable name'),
    t('Value'),
    t('Description'),
  );
  $rows = array();
  foreach ($variables['items'] AS $i => $item) {
    $description = array();
    if (!empty($item['description'])) {
      $description[] = sprintf('<span class="description">%s</span>', $item['description']);
    }
    if (!empty($item['documentation_url'])) {
      $item += array('documentation_text' => t('Read the documentation »'));
      $description[] = sprintf('<span class="docs">%s</span>', l($item['documentation_text'], $item['documentation_url'], array('attributes' => array('target' => '_blank'))));
    }
    $rows[] = array(
      $item['label'],
      $item['value'],
      implode(" ", $description),
    );
  }
  return theme('table', array('header' => $header, 'rows' => $rows));
  $build = array();
  if (!empty($variables['title'])) {
    $build['title'] = array('#markup' => sprintf('<h3>%s</h3>', $variables['title']), '#weight' => -100);
  }
  foreach ($variables['items'] AS $i => $item) {
    $build[$i] = array(
      '#theme' => 'msm_variables_help_item',
      '#item' => $item,
      '#weight' => isset($item['weight']) ? $item['weight'] : 0,
    );
  }
  return '<div class="msm-variables-help">' . drupal_render($build) . '</div>';
}

function theme_msm_schema_result_tree($variables) {
  $fields = msm_schema_flatten_fields($variables['schema'], MSM_SCHEMA_FLATTEN_ALL);
  $result = $variables['result'];
  $header = array(t('Name'), t('Type'), t('Value'));
  $display_usage = FALSE;
  if (isset($fields['_id']['totalOccurrences'])) {
    $display_usage = TRUE;
    $header[] = sprintf('<span title="%s">%s</span>', t('Total Occurrences/Percent Container'), t('Usage'));
  }
  foreach ($fields AS $field_name => $field_info) {
    $display = theme('msm_schema_field_label', array(
      'field_name' => $field_name,
      'field_info' => $field_info,
    ));
    $level = count($field_info['parents'])-1;
    for ($i = 0; $i < $level; $i++) {
      $display = '<span class="indent">&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;</span>' . $display;
    }
    if (in_array($field_info['type'], array('group', 'array'))) {
      $display = sprintf('<strong>%s</strong>', $display);
      if (!empty($field_info['display']['description'])) {
        $display .= sprintf('&nbsp;-&nbsp;<em>%s</em>', $field_info['display']['description']);
      }
      $row = array(array('colspan' => 3, 'data' => $display));
    }
    else {
      $row = array(
        $display,
        $field_info['type'],
        drupal_array_get_nested_value($result, $field_info['parents'])
      );
    }
    if ($display_usage) {
      $row[] = sprintf('%s%%', number_format($field_info['percentContaining']));
    }
    $rows[] = $row;
  }
  return theme('table', array('header' => $header, 'rows' => $rows));
}

function theme_msm_schema_field_label($variables) {
  $field_name = $variables['field_name'];
  $field_info = $variables['field_info'];
  $key = empty($variables['key']) ? 'title' : $variables['key'];
  $title = empty($field_info['display'][$key]) ? $field_name : $field_info['display'][$key];
  if (empty($field_info['display']['description'])) {
    if ($key !== 'title') {
      $description = $title;
    }
    else {
      $description = $field_name;
    }
  }
  else $description = $field_info['display']['description'];
  return sprintf('<span class="msm-schema-field-label" title="%s" alt="%s">%s</span>',
    $description,
    $description,
    $title
  );
}

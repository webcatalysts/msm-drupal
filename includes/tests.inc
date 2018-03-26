<?php

function msm_tests_page() {
  $build = array();
  $build['table'] = array(
    '#theme' => 'table',
    '#header' => array(
      'Test Name',
      'Test Collection',
      'Test Database',
      'Result',
      'Error',
      'Actions',
    ),
    '#rows' => array(),
  );
  $tests = msm_get('admin/tests');
  $counts = array('pass' => 0, 'fail' => 0, 'nr' => 0);
  foreach ($tests AS $test) {
    $actions = array();
    if (!isset($test['pass'])) {
      $result = 'never ran';
      $counts['nr']++;
    }
    else {
      $result = $test['pass'] ? 'Pass' : 'Fail';
      if ($test['pass']) $counts['pass']++;
      else $counts['fail']++;
    }
    $row = array(
      $test['_id'],
      $test['collection'],
      $test['database'],
      $result,
      empty($test['error']) ? '-' : $test['error'],
      render($actions),
    );
    $build['table']['#rows'][] = $row;
  }
  $build['summary'] = array(
    '#weight' => -100,
    '#markup' => sprintf('<div class="tests-summary">%s</div>', t('@pass Passing, @fail Failing, @nr Never Ran of @total Total Tests', array(
      '@total' => count($tests),
      '@pass' => $counts['pass'],
      '@fail' => $counts['fail'],
      '@nr' => $counts['nr'],
    ))),
  );
  return $build;
}

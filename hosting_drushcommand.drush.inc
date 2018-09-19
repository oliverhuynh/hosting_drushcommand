<?php
/**
 * @file
 */

/**
 * Map values of site node into command line arguments
 */
function drush_hosting_drushcommand_pre_hosting_task($task) {
  $task = &drush_get_context('HOSTING_TASK');
  if ($task->task_type == 'drushcommand') {
    $task->args[1] = $task->task_args['drushcommand'];
    $task->args[2] = $task->task_args['drushargument'];
    $task->args[3] = $task->task_args['drushoptions'];
    $task->args[4] = isset($task->task_args['triggeronremote']) && $task->task_args['triggeronremote'] ? 1 : 0;
  }

  $commands = load_customdrushcommand();
  foreach ($commands as $command) {
    if ($task->task_type == $command['id']) {
      $num = 1;
      foreach ($command['arguments'] as $key => $arg) {
        $task->args[$num] = '"'. addslashes($task->task_args[$key]) .'"';
        $num++;
      }
      foreach ($command['options'] as $key => $arg) {
        $task->options[$key] = $task->task_args[$key];
      }
    }
  }
}

/**
 * Implements hook_drush_command().
 */
function hosting_drushcommand_drush_command() {
  $items = array();

  $items['provision-drushcommand'] = array(
    'description' => 'drushcommand a site.',
    'examples' => array(
      'drush @site provision-drushcommand "cc all" "" "" 1' => 'Clear all cache remotely',
    ),
    'arguments' => array(
      'drushcommand' => array(
        'description' => 'Drush command',
      ),
      'drushargument' => array(
        'description' => 'Drush argument',
      ),
      'drushoptions' => array(
        'description' => 'Drush options',
      ),
      'triggeronremote' => array(
        'description' => 'Trigger on remote',
      ),
    ),
    'bootstrap' => DRUSH_BOOTSTRAP_DRUPAL_ROOT
  );


  $results = load_customdrushcommand();
  foreach ($results as $ret) {
    $items['provision-' . $ret['id']] = array(
      'description' => $ret['description'],
      'examples' => $ret['examples'],
      'arguments' => $ret['arguments'],
      'bootstrap' => $ret['bootstrap'],
    );
  }

  return $items;
}

function load_customdrushcommand() {
  $return = array();
  $list = drush_commandfile_list();
  foreach ($list as $commandfile => $path) {
    if (drush_command_hook($commandfile, 'customdrushcommand')) {
      $function = $commandfile . '_customdrushcommand';
      $results = $function();
      $return += $results;
    }
  }
  return $return;
}

/**
 * Drush Provision drushcommand task callback.
 *
 * Invoke migrate files.
 */
function drush_hosting_drushcommand_execcustom($id) {
  $commands = load_customdrushcommand();
  $command = $commands[$id];
  $vars = array_keys($command['arguments']);
  $options = array_keys($command['options']);
  $map = $command['map'];

  // Build vars and options
  $varstoken = array();
  $items = func_get_args();
  array_shift($items);
  // Remove quote out
  foreach ($items as &$item) {
    $item = strpos($item, '"') === 0 ? substr($item, 1, strlen($item) - 2) : $item;
  }
  $optionstoken = array();
  foreach ($vars as $var) {
    $varstoken[$var] = array_shift($items);
  }
  foreach ($options as $option) {
    $opt = drush_get_option($option, '');
    $opts = array_map('trim', explode(',', $opt));
    $opts = array_filter($opts);
    $optionstoken[$option] = $opts;
  }
  $command = hosting_drushcommand_build_command($map, $varstoken, $optionstoken);

  if (_exec_bash($command, array(), array(), 0)) {
    return ;
  }
  return drush_log('Custom drushcommand failed:' . $command, 'error');
}

function hosting_drushcommand_build_command($map, $varstoken, $optionstoken) {
  // Cleanup for hook_tokens
  module_implements('token', $sort = FALSE, TRUE);
  return token_replace($map, array('site' => _get_site_alias(), 'vars' => $varstoken, 'opts' => $optionstoken));
}
function hdc_addroot($relativedir) {
  $site_record = _get_site_alias();
  return $relativedir ? $site_record['root'] . '/' . $relativedir : $site_record['root'];
}

function hdc_options($data, $arg) {
  $data = is_array($data) ? $data : array($data);
  $data = array_filter($data);
  foreach ($data as &$item) {
    $item = $arg . '=' . $item;
  }
  return implode(' ', $data);
}

/**
 * Implement of hook_tokens
 * We invite dblog
 */
function dblog_tokens($type, $tokens, array $data = array(), array $options = array()) {
  $replacements = array();
  if ($type == 'site' || $type == 'vars' || $type == 'opts') {
    foreach ($tokens as $name => $original) {
      $names = explode(":", $name);
      $names += array('', '', FALSE);
      list ($name, $callback, $arg) = $names;
      $val = isset($data[$type][$name]) ? $data[$type][$name] : '';
      $replacements[$original] = _execute_token_callback($names, $val);

    }
  }
  return $replacements;
}

function _execute_token_callback($names, $val) {
  $names += array('', '', FALSE, FALSE);
  list ($name, $callback, $arg, $nextcallback) = $names;
  $ret = $val;

  if (function_exists($callback)) {
    $callnext = FALSE;
    if (function_exists($arg)) {
      array_shift($names);
      array_shift($names);
      array_unshift($names, $name);
      $callnext = TRUE;
    }
    else {
      if (function_exists($nextcallback)) {
        array_shift($names);
        array_shift($names);
        array_shift($names);
        array_unshift($names, $name);
        $callnext = TRUE;
      }
    }
    $ret = $arg === FALSE || $callnext ? $callback($ret) : $callback($ret, $arg);
    if ($callnext) {
      return _execute_token_callback($names, $ret);
    }
  }
  return $ret;
}

/**
 * Drush Provision drushcommand task callback.
 *
 * Invokes drush command
 */
function drush_hosting_drushcommand_provision_drushcommand($drushcommand, $arguments = '', $options = '', $triggeronremote = 0) {
  try {
    $arguments = json_decode($arguments);
  }
  catch (Exception $e) {
    $arguments = array();
  }
  try {
    $options = json_decode($options);
  }
  catch (Exception $e) {
    $options = array();
  }

  drush_log('Implementing drush command: ' . print_r(func_get_args(), TRUE), 'ok');
  if (!$drushcommand) {
    return ;
  }
  if ($triggeronremote) {
    if (_exec_on_remote($drushcommand, $arguments, $options)) {
      return ;
    }
    return drush_log('Exec '. $drushcommand .' error', 'error');
  }
  $d_name = d()->name;
  provision_backend_invoke($d_name, $drushcommand, $arguments, $options);
}

function _exec_on_remote($drushcommand, $arguments, $options) {
  return _exec_bash('drush ' . $drushcommand, $arguments, $options, 1);
}

function _get_site_alias() {
  $alias = drush_get_context('DRUSH_TARGET_SITE_ALIAS');
  $site = drush_sitealias_get_record($alias);
  $platform = drush_sitealias_get_record($site['platform']);
  $web_server = drush_sitealias_get_record($platform['web_server']);
  $site = array(
    'remote-host' => $web_server['remote_host'],
    'remote-user' => $web_server['script_user'],
  ) + $site;
  return $site;
}

function _exec_bash($bashcommand, $arguments, $options, $triggeronremote = 0) {
  $bash = trim($bashcommand . ' ' . implode(' ', $arguments) . ' ' . implode(' ', $options));
  $site = _get_site_alias();
  $origsite = $site;
  if (!$triggeronremote) {
    unset($site['remote-host'], $site['remote-user']);
  }
  $ret = ra_up_shell_exec($site, $bash);
  list($success, $bash, $output) = $ret;
  $type = 'ok';
  if (!$success) {
    $type = 'error';
  }
  if (is_array($output)) {
    foreach ($output as $line) {
      if (trim($line)) {
        drush_log($line, $type);
      }
    }
  }
  else {
    drush_log(print_r($ret, TRUE), $type);
  }
  return $success;
}

function ra_up_shell_exec($site_record, $bash) {
  $bash = 'cd ' . drush_escapeshellarg($site_record['root']) . ' && ' . $bash;
  $exec = _drush_backend_generate_command($site_record, $bash);
  drush_log('(ra_up_shell_exec) Executing: ' . $bash, 'ok');
  $success = drush_shell_exec($exec);
  $output = drush_shell_exec_output();
  return array($success, $bash, $output);
}
